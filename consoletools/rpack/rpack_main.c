#include "rpack/rpack_encoder.h"
#include "rpack/rpack_val.h"
#include "rpack/rpack_pipeline.h"

#include "rlgame.base/cstl/cstl_log.h"
#include "rlgame.base/cstl/cstl_string.h"
#include "rlgame.base/cstl/cstl_array.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
#include "rlgame.base/cstl/cstl_thread.h"
#include "rlgame.base/cstl/cstl_atomic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static void
R_Pack_LogSettingsurationWarnings (const struct R_Pack_EncoderSettings* pSettings)
{
    uint64_t atlasBytes = (uint64_t)pSettings->maxAtlasWidth * pSettings->maxAtlasHeight * 2;
    if (atlasBytes > 32ULL * 1024ULL * 1024ULL)
    {
        R_CSTL_LOG_WARN (
            "Atlas limit is %ux%u (%.1f MiB); large images may exhaust the rpack heap",
            pSettings->maxAtlasWidth,
            pSettings->maxAtlasHeight,
            (double)atlasBytes / (1024.0 * 1024.0));
    }
    if (pSettings->border != 0)
    {
        R_CSTL_LOG_WARN (
            "Border size %u was requested but border pixels are not encoded yet",
            pSettings->border);
    }
    if (pSettings->powerOfTwo)
    {
        R_CSTL_LOG_WARN ("Power-of-two atlas sizing was requested but is not implemented yet");
    }
    if (pSettings->enableRotation)
    {
        R_CSTL_LOG_WARN ("Texture rotation was requested but is not implemented yet");
    }
    if (pSettings->alphaThreshold > 0.0f)
    {
        R_CSTL_LOG_WARN (
            "Alpha threshold %.3f was requested but alpha masking is not implemented yet",
            pSettings->alphaThreshold);
    }
}

static void
R_Pack_PrintHelp ()
{
    printf ("Usage\n");
    printf ("  [options] -o OUTPUT.rpack INPUT1 [INPUT2 ...]\n\n");

    printf ("Options\n");
    printf ("  -o, --output FILE           = Output RPACK file path (required).\n");
    printf ("  -w, --width SIZE            = Maximum atlas width in pixels (default: 4096).\n");
    printf ("  -H, --height SIZE           = Maximum atlas height in pixels (default: 4096).\n");
    printf ("  -p, --padding SIZE          = Padding between textures in pixels (default: 1).\n");
    printf ("  -b, --border SIZE           = Border size around textures in pixels (default: 0).\n");
    printf ("  -t, --threshold FLOAT       = Color similarity threshold 0.0-1.0 (default: 0.1).\n");
    printf ("                                Lower values = more colors, higher = fewer colors.\n");
    printf ("  -j, --workers COUNT         = Number of worker threads (default: 0 = auto).\n");
    printf ("                                et to 1 for single-threaded mode.\n");
    printf ("  -v, --verbose               = Enable verbose output with detailed progress.\n");
    printf ("  -q, --quiet                 = Suppress all non-error output.\n");
    printf ("  --power-of-two              = Force atlas dimensions to be power of two.\n");
    printf ("  --rotate                    = Enable texture rotation for better packing.\n");
    printf ("  --alpha-threshold FLOAT     = Alpha threshold 0.0-1.0 (default: 0.0).\n");
    printf ("                               Pixels below this alpha are considered transparent.\n");
    printf ("  --max-textures COUNT        = Maximum number of textures to pack (default: unlimited).\n");
    printf ("  --mipmap                    = Generate 64x64 through 1x1 RPACK variants.\n");
    printf ("  --help                      = Print this help message and exit.\n");
    printf ("  --version                   = Print version information and exit.\n\n");
}

static int
R_Pack_ParseArguments (
    int                          argc,
    char**                       argv,
    struct R_Pack_EncoderSettings* pSettings,
    char**                       ppOutputPath,
    struct R_CSTL_Array**        ppInputPaths,
    int*                         pVerbose,
    int*                         pQuiet,
    int*                         pMipmap)
{
    *ppOutputPath = NULL;
    *ppInputPaths = R_CSTL_NewArray ();
    if (!*ppInputPaths)
    {
        return -1;
    }
    *pVerbose = 0;
    *pQuiet = 0;
    *pMipmap = 0;

    if (pSettings)
    {
        pSettings->maxAtlasWidth = R_PACK_DEFAULT_MAX_ATLAS_WIDTH;
        pSettings->maxAtlasHeight = R_PACK_DEFAULT_MAX_ATLAS_HEIGHT;
        pSettings->padding = R_PACK_DEFAULT_PADDING;
        pSettings->border = R_PACK_DEFAULT_BORDER;
        pSettings->similarityThreshold = R_PACK_DEFAULT_SIMILARITY_THRESHOLD;
        pSettings->alphaThreshold = R_PACK_DEFAULT_ALPHA_THRESHOLD;
        pSettings->workerCount = R_PACK_DEFAULT_WORKER_COUNT;
        pSettings->maxTextures = R_PACK_DEFAULT_MAX_TEXTURES;
        pSettings->powerOfTwo = R_PACK_DEFAULT_POWER_OF_TWO;
        pSettings->enableRotation = R_PACK_DEFAULT_ENABLE_ROTATION;
    }
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp (argv[i], "--help") == 0 || strcmp (argv[i], "-h") == 0)
        {
            R_Pack_PrintHelp ();
            return 1;
        }
        else if (strcmp (argv[i], "-o") == 0 || strcmp (argv[i], "--output") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf (stderr, "\033[1;31mError: --output requires an argument\033[0m\n");
                return -1;
            }
            *ppOutputPath = argv[++i];
        }
        else if (strcmp (argv[i], "-w") == 0 || strcmp (argv[i], "--width") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf (stderr, "\033[1;31mError: --width requires an argument\033[0m\n");
                return -1;
            }
            if (pSettings)
            {
                pSettings->maxAtlasWidth = (uint32_t)atoi (argv[++i]);
            }
        }
        else if (strcmp (argv[i], "-H") == 0 || strcmp (argv[i], "--height") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf (stderr, "\033[1;31mError: --height requires an argument\033[0m\n");
                return -1;
            }
            if (pSettings)
            {
                pSettings->maxAtlasHeight = (uint32_t)atoi (argv[++i]);
            }
        }
        else if (strcmp (argv[i], "-p") == 0 || strcmp (argv[i], "--padding") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf (stderr, "\033[1;31mError: --padding requires an argument\033[0m\n");
                return -1;
            }
            if (pSettings)
            {
                pSettings->padding = (uint32_t)atoi (argv[++i]);
            }
        }
        else if (strcmp (argv[i], "-t") == 0 || strcmp (argv[i], "--threshold") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf (stderr, "\033[1;31mError: --threshold requires an argument\033[0m\n");
                return -1;
            }
            if (pSettings)
            {
                pSettings->similarityThreshold = (float)atof (argv[++i]);
            }
        }
        else if (strcmp (argv[i], "-j") == 0 || strcmp (argv[i], "--workers") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf (stderr, "\033[1;31mError: --workers requires an argument\033[0m\n");
                return -1;
            }
            if (pSettings)
            {
                pSettings->workerCount = (uint32_t)atoi (argv[++i]);
            }
        }
        else if (strcmp (argv[i], "-b") == 0 || strcmp (argv[i], "--border") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf (stderr, "\033[1;31mError: --border requires an argument\033[0m\n");
                return -1;
            }
            if (pSettings)
            {
                pSettings->border = (uint32_t)atoi (argv[++i]);
            }
        }
        else if (strcmp (argv[i], "-v") == 0 || strcmp (argv[i], "--verbose") == 0)
        {
            *pVerbose = 1;
        }
        else if (strcmp (argv[i], "-q") == 0 || strcmp (argv[i], "--quiet") == 0)
        {
            *pQuiet = 1;
        }
        else if (strcmp (argv[i], "--power-of-two") == 0)
        {
            if (pSettings)
            {
                pSettings->powerOfTwo = 1;
            }
        }
        else if (strcmp (argv[i], "--rotate") == 0)
        {
            if (pSettings)
            {
                pSettings->enableRotation = 1;
            }
        }
        else if (strcmp (argv[i], "--alpha-threshold") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf (stderr, "\033[1;31mError: --alpha-threshold requires an argument\033[0m\n");
                return -1;
            }
            if (pSettings)
            {
                pSettings->alphaThreshold = (float)atof (argv[++i]);
            }
        }
        else if (strcmp (argv[i], "--max-textures") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf (stderr, "\033[1;31mError: --max-textures requires an argument\033[0m\n");
                return -1;
            }
            if (pSettings)
            {
                pSettings->maxTextures = (uint32_t)atoi (argv[++i]);
            }
        }
        else if (strcmp (argv[i], "--mipmap") == 0)
        {
            *pMipmap = 1;
        }
        else if (strcmp (argv[i], "--version") == 0)
        {
            printf ("RPACK Texture Packer v1.0.0\n");
            return 1;
        }
        else if (argv[i][0] == '-')
        {
            fprintf (stderr, "\033[1;31mError: Unknown option '%s'\033[0m\n", argv[i]);
            return -1;
        }
        else
        {
            size_t len = strlen (argv[i]) + 1;
            int    result = R_CSTL_ArrayPushData (*ppInputPaths, (const uint8_t*)argv[i], len);
            if (result != R_CSTL_OK)
            {
                fprintf (stderr, "\033[1;31mError: Out of memory\033[0m\n");
                return -1;
            }
        }
    }
    if (!*ppOutputPath)
    {
        fprintf (stderr, "\033[1;31mError: --output is required\033[0m\n");
        return -1;
    }
    if (R_CSTL_ArrayLength (*ppInputPaths) == 0)
    {
        fprintf (stderr, "\033[1;31mError: At least one input file is required\033[0m\n");
        return -1;
    }
    return 0;
}

#define R_PACK_HEAP_SIZE 268435456ul

int
main (int argc, char** argv)
{
    if (R_CSTL_LogInit () != R_CSTL_OK)
    {
        return EXIT_FAILURE;
    }
    R_CSTL_LogSetFlags (R_CSTL_LogGetFlags () | R_CSTL_LOG_FLAG_ENABLE_COLORS);
    if (R_CSTL_HeapInit (R_PACK_HEAP_SIZE) != R_CSTL_OK)
    {
        R_CSTL_LOG_FATAL ("Failed to initialize RPACK heap\n");
        return EXIT_FAILURE;
    }
    if (argc == 1)
    {
        R_Pack_PrintHelp ();
        R_CSTL_HeapShutdown ();
        return EXIT_SUCCESS;
    }
    struct R_Pack_EncoderSettings config = {0};
    char*                       pOutputPath = NULL;
    struct R_CSTL_Array*        pInputPaths = NULL;
    struct R_Pack_Encoder*      pEncoder = NULL;
    int                         verbose = 0;
    int                         quiet = 0;
    int                         mipmap = 0;
    int                         result = EXIT_FAILURE;

    int parseResult
        = R_Pack_ParseArguments (argc, argv, &config, &pOutputPath, &pInputPaths, &verbose, &quiet, &mipmap);
    if (parseResult == 1)
    {
        R_CSTL_DeleteArray (pInputPaths);
        R_CSTL_HeapShutdown ();
        return EXIT_SUCCESS;
    }
    if (parseResult < 0)
    {
        R_CSTL_DeleteArray (pInputPaths);
        R_CSTL_HeapShutdown ();
        return EXIT_FAILURE;
    }
    if (verbose)
    {
        R_CSTL_LogSetMinLevel (R_CSTL_LOG_LEVEL_TRACE);
    }
    (void)quiet;
    R_CSTL_LOG_INFO ("Packing assets as RPACK");
    R_Pack_LogSettingsurationWarnings (&config);

    pEncoder = R_Pack_NewEncoder (&config);
    if (!pEncoder)
    {
        goto r_cleanup_logging;
    }
    uint32_t successCount = R_Pack_EncodeInputImagesThreaded (pEncoder, pInputPaths, 0, config.workerCount);
    if (successCount == 0)
    {
        R_CSTL_LOG_ERROR ("No images were processed");
        goto r_cleanup_encoder;
    }
    size_t      inputCount = 0;
    size_t      inputOffset = 0;
    size_t      inputBytes = R_CSTL_ArrayLength (pInputPaths);
    const char* pInputData = (const char*)R_CSTL_ArrayData (pInputPaths);
    while (inputOffset < inputBytes)
    {
        size_t pathLength = strnlen (pInputData + inputOffset, inputBytes - inputOffset);
        if (pathLength == inputBytes - inputOffset) break;
        ++inputCount;
        inputOffset += pathLength + 1;
    }
    R_CSTL_LOG_INFO ("Processed %u/%zu images", successCount, inputCount);

    if (R_Pack_EncodeAndWrite (pEncoder, pOutputPath) != 0)
    {
        goto r_cleanup_encoder;
    }
    if (mipmap && R_Pack_EncodeMipmapVariants (&config, pInputPaths, pOutputPath) != 0)
    {
        R_CSTL_LOG_ERROR ("Mipmap generation failed for every requested level");
        goto r_cleanup_encoder;
    }
    R_CSTL_LOG_INFO ("Packing completed");
    result = EXIT_SUCCESS;
r_cleanup_encoder:
    R_Pack_DeleteEncoder (pEncoder);
r_cleanup_logging:
    R_CSTL_LogShutdown ();
r_cleanup_input_paths:
    R_CSTL_DeleteArray (pInputPaths);
    R_CSTL_HeapShutdown ();
    return result;
}
