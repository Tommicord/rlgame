#include "rpack/rpack_encoder.h"
#include "rlgame.base/cstl/cstl_log.h"
#include "rlgame.base/cstl/cstl_string.h"
#include "rlgame.base/cstl/cstl_array.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
R_Pack_PrintHelp ()
{
    printf ("Usage\n");
    printf ("  [options] -o OUTPUT.rpack INPUT1 [INPUT2 ...]\n\n");

    printf ("Description\n");
    printf ("  RPACK is a texture packing tool that combines multiple images into a single\n");
    printf ("  atlas texture with optimized layout. It generates a RPACK file containing\n");
    printf ("  the atlas image and metadata for texture coordinates.\n\n");

    printf ("Options\n");
    printf ("  -o, --output FILE           = Output RPACK file path (required).\n");
    printf ("  -w, --width SIZE            = Maximum atlas width in pixels (default: 4096).\n");
    printf ("  -h, --height SIZE           = Maximum atlas height in pixels (default: 4096).\n");
    printf ("  -p, --padding SIZE          = Padding between textures in pixels (default: 1).\n");
    printf ("  -b, --border SIZE           = Border size around textures in pixels (default: 0).\n");
    printf ("  -t, --threshold FLOAT       = Color similarity threshold 0.0-1.0 (default: 0.1).\n");
    printf ("                               Lower values = more colors, higher = fewer colors.\n");
    printf ("  -j, --workers COUNT         = Number of worker threads (default: 0 = auto).\n");
    printf ("                               Set to 1 for single-threaded mode.\n");
    printf ("  -c, --colors                = Enable colored console output.\n");
    printf ("  -v, --verbose               = Enable verbose output with detailed progress.\n");
    printf ("  -q, --quiet                 = Suppress all non-error output.\n");
    printf ("  --power-of-two              = Force atlas dimensions to be power of two.\n");
    printf ("  --rotate                    = Enable texture rotation for better packing.\n");
    printf ("  --alpha-threshold FLOAT     = Alpha threshold 0.0-1.0 (default: 0.0).\n");
    printf ("                               Pixels below this alpha are considered transparent.\n");
    printf ("  --max-textures COUNT        = Maximum number of textures to pack (default: unlimited).\n");
    printf ("  --help                      = Print this help message and exit.\n");
    printf ("  --version                   = Print version information and exit.\n\n");
}

static int
R_Pack_ParseArguments (
    int                         argc,
    char**                      argv,
    struct R_PackEncoderConfig* pConfig,
    char**                      ppOutputPath,
    struct R_CSTL_Array**       ppInputPaths,
    int*                        pEnableColors,
    int*                        pVerbose,
    int*                        pQuiet)
{
    *ppOutputPath = NULL;
    *ppInputPaths = R_CSTL_NewArray ();
    if (!*ppInputPaths)
    {
        return -1;
    }
    *pEnableColors = 0;
    *pVerbose = 0;
    *pQuiet = 0;

    if (pConfig)
    {
        pConfig->maxAtlasWidth = R_RPACK_DEFAULT_MAX_ATLAS_WIDTH;
        pConfig->maxAtlasHeight = R_RPACK_DEFAULT_MAX_ATLAS_HEIGHT;
        pConfig->padding = R_RPACK_DEFAULT_PADDING;
        pConfig->border = R_RPACK_DEFAULT_BORDER;
        pConfig->similarityThreshold = R_RPACK_DEFAULT_SIMILARITY_THRESHOLD;
        pConfig->alphaThreshold = R_RPACK_DEFAULT_ALPHA_THRESHOLD;
        pConfig->workerCount = R_RPACK_DEFAULT_WORKER_COUNT;
        pConfig->maxTextures = R_RPACK_DEFAULT_MAX_TEXTURES;
        pConfig->powerOfTwo = R_RPACK_DEFAULT_POWER_OF_TWO;
        pConfig->enableRotation = R_RPACK_DEFAULT_ENABLE_ROTATION;
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
            if (pConfig)
            {
                pConfig->maxAtlasWidth = (uint32_t)atoi (argv[++i]);
            }
        }
        else if (strcmp (argv[i], "-h") == 0 || strcmp (argv[i], "--height") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf (stderr, "\033[1;31mError: --height requires an argument\033[0m\n");
                return -1;
            }
            if (pConfig)
            {
                pConfig->maxAtlasHeight = (uint32_t)atoi (argv[++i]);
            }
        }
        else if (strcmp (argv[i], "-p") == 0 || strcmp (argv[i], "--padding") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf (stderr, "\033[1;31mError: --padding requires an argument\033[0m\n");
                return -1;
            }
            if (pConfig)
            {
                pConfig->padding = (uint32_t)atoi (argv[++i]);
            }
        }
        else if (strcmp (argv[i], "-t") == 0 || strcmp (argv[i], "--threshold") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf (stderr, "\033[1;31mError: --threshold requires an argument\033[0m\n");
                return -1;
            }
            if (pConfig)
            {
                pConfig->similarityThreshold = (float)atof (argv[++i]);
            }
        }
        else if (strcmp (argv[i], "-j") == 0 || strcmp (argv[i], "--workers") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf (stderr, "\033[1;31mError: --workers requires an argument\033[0m\n");
                return -1;
            }
            if (pConfig)
            {
                pConfig->workerCount = (uint32_t)atoi (argv[++i]);
            }
        }
        else if (strcmp (argv[i], "-c") == 0 || strcmp (argv[i], "--colors") == 0)
        {
            *pEnableColors = 1;
        }
        else if (strcmp (argv[i], "-b") == 0 || strcmp (argv[i], "--border") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf (stderr, "\033[1;31mError: --border requires an argument\033[0m\n");
                return -1;
            }
            if (pConfig)
            {
                pConfig->border = (uint32_t)atoi (argv[++i]);
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
            if (pConfig)
            {
                pConfig->powerOfTwo = 1;
            }
        }
        else if (strcmp (argv[i], "--rotate") == 0)
        {
            if (pConfig)
            {
                pConfig->enableRotation = 1;
            }
        }
        else if (strcmp (argv[i], "--alpha-threshold") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf (stderr, "\033[1;31mError: --alpha-threshold requires an argument\033[0m\n");
                return -1;
            }
            if (pConfig)
            {
                pConfig->alphaThreshold = (float)atof (argv[++i]);
            }
        }
        else if (strcmp (argv[i], "--max-textures") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf (stderr, "\033[1;31mError: --max-textures requires an argument\033[0m\n");
                return -1;
            }
            if (pConfig)
            {
                pConfig->maxTextures = (uint32_t)atoi (argv[++i]);
            }
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

static int
R_Pack_LoadImgAsset (const char* pPath, struct R_PackInputImage* pImage, uint8_t** ppPixelBuffer)
{
    (void)pPath;
    (void)pImage;
    (void)ppPixelBuffer;

    fprintf (stderr, "\033[1;33mWarning: Image loading from file not yet implemented\033[0m\n");
    fprintf (stderr, "\033[1;33mThis is a placeholder for runtime validation\033[0m\n");

    return -1;
}

static int
R_Pack_InitializeLogging (int enableColors)
{
    if (R_CSTL_LogInit () != 0)
    {
        fprintf (stderr, "\033[1;31mError: Failed to initialize logging\033[0m\n");
        return -1;
    }
    if (enableColors)
    {
        R_CSTL_LogSetFlags (R_CSTL_LogGetFlags () | R_CSTL_LOG_FLAG_ENABLE_COLORS);
    }
    return 0;
}

static struct R_PackEncoder*
R_Pack_CreateEncoder (const struct R_PackEncoderConfig* pConfig)
{
    struct R_PackEncoder* pEncoder = R_Pack_NewEncoder (pConfig);
    if (!pEncoder)
    {
        R_CSTL_LOG_ERROR ("Failed to create encoder");
    }
    return pEncoder;
}

static uint32_t
R_Pack_EncodeInputImages (struct R_PackEncoder* pEncoder, const struct R_CSTL_Array* pInputPaths)
{
    uint32_t successCount = 0;
    size_t   inputCount = R_CSTL_ArrayLength (pInputPaths) / sizeof (char*);
    size_t   offset = 0;

    for (size_t i = 0; i < inputCount; ++i)
    {
        char* pPath = NULL;
        R_CSTL_ArrayTypedAt (pInputPaths, char*, offset, &pPath);
        offset += strlen (pPath) + 1;

        R_CSTL_LOG_INFO ("Processing input %zu: %s", i + 1, pPath);

        uint8_t*                pPixelBuffer = NULL;
        struct R_PackInputImage image = {0};

        int loadResult = R_Pack_LoadImgAsset (pPath, &image, &pPixelBuffer);
        if (loadResult < 0)
        {
            R_CSTL_LOG_ERROR ("Failed to load image: %s", pPath);
            continue;
        }

        enum R_PackError err = R_Pack_EncoderAddImage (pEncoder, &image);
        if (err != R_RPACK_OK)
        {
            R_CSTL_LOG_ERROR ("Failed to add image '%s': %s", pPath, R_PackErrorToString (err));
            if (pPixelBuffer)
            {
                R_CSTL_HeapFree (pPixelBuffer);
            }
            continue;
        }
        successCount++;

        if (pPixelBuffer)
        {
            R_CSTL_HeapFree (pPixelBuffer);
        }
    }

    return successCount;
}

static int
R_Pack_EncodeAndWrite (struct R_PackEncoder* pEncoder, const char* pOutputPath)
{
    uint64_t requiredSize = R_Pack_EncoderGetRequiredSize (pEncoder);
    R_CSTL_LOG_INFO ("Required output size: %llu bytes", (unsigned long long)requiredSize);

    uint8_t* pOutputBuffer = (uint8_t*)R_CSTL_HeapAlloc (requiredSize);
    if (!pOutputBuffer)
    {
        R_CSTL_LOG_ERROR ("Failed to allocate output buffer");
        return -1;
    }

    uint64_t         bytesWritten = 0;
    enum R_PackError encodeErr = R_Pack_EncoderEncode (pEncoder, pOutputBuffer, requiredSize, &bytesWritten);
    if (encodeErr != R_RPACK_OK)
    {
        R_CSTL_LOG_ERROR ("Encoding failed: %s", R_PackErrorToString (encodeErr));
        R_CSTL_HeapFree (pOutputBuffer);
        return -1;
    }

    FILE* pOutputFile = fopen (pOutputPath, "wb");
    if (!pOutputFile)
    {
        R_CSTL_LOG_ERROR ("Failed to open output file: %s", pOutputPath);
        R_CSTL_HeapFree (pOutputBuffer);
        return -1;
    }

    size_t written = fwrite (pOutputBuffer, 1, bytesWritten, pOutputFile);
    fclose (pOutputFile);

    if (written != bytesWritten)
    {
        R_CSTL_LOG_ERROR ("Failed to write complete output file");
        R_CSTL_HeapFree (pOutputBuffer);
        return -1;
    }

    R_CSTL_LOG_INFO ("Successfully wrote %llu bytes to %s", (unsigned long long)bytesWritten, pOutputPath);
    R_CSTL_HeapFree (pOutputBuffer);
    return 0;
}

static void
R_Pack_CleanupResources (struct R_PackEncoder* pEncoder, struct R_CSTL_Array* pInputPaths)
{
    if (pEncoder)
    {
        R_Pack_DeleteEncoder (pEncoder);
    }
    if (pInputPaths)
    {
        R_CSTL_DeleteArray (pInputPaths);
    }
    R_CSTL_LogShutdown ();
}

int
main (int argc, char** argv)
{
    if (argc == 1)
    {
        R_Pack_PrintHelp ();
        return EXIT_SUCCESS;
    }

    struct R_PackEncoderConfig config = {0};
    char*                      pOutputPath = NULL;
    struct R_CSTL_Array*       pInputPaths = NULL;
    int                        enableColors = 0;
    struct R_PackEncoder*      pEncoder = NULL;
    int                        verbose = 0;
    int                        quiet = 0;
    int                        result = EXIT_FAILURE;

    int parseResult = R_Pack_ParseArguments (
        argc,
        argv,
        &config,
        &pOutputPath,
        &pInputPaths,
        &enableColors,
        &verbose,
        &quiet);
    if (parseResult == 1)
    {
        return EXIT_SUCCESS;
    }
    if (parseResult < 0)
    {
        return EXIT_FAILURE;
    }
    if (R_Pack_InitializeLogging (enableColors) != 0)
    {
        return EXIT_FAILURE;
    }
    R_CSTL_LOG_INFO ("RPACK: Packing assets");

    pEncoder = R_Pack_CreateEncoder (&config);
    if (!pEncoder)
    {
        goto r_cleanup_logging;
    }
    uint32_t successCount = R_Pack_EncodeInputImages (pEncoder, pInputPaths);
    if (successCount == 0)
    {
        R_CSTL_LOG_ERROR ("No images were successfully processed");
        goto r_cleanup_encoder;
    }
    size_t inputCount = R_CSTL_ArrayLength (pInputPaths) / sizeof (char*);
    R_CSTL_LOG_INFO ("Successfully processed %u/%zu images", successCount, inputCount);

    if (R_Pack_EncodeAndWrite (pEncoder, pOutputPath) != 0)
    {
        goto r_cleanup_encoder;
    }
    R_CSTL_LOG_INFO ("RPACK: Packing completed successfully");
    result = EXIT_SUCCESS;
r_cleanup_encoder:
    if (pEncoder)
    {
        R_Pack_DeleteEncoder (pEncoder);
    }

r_cleanup_logging:
    R_CSTL_LogShutdown ();

r_cleanup_input_paths:
    if (pInputPaths)
    {
        R_CSTL_DeleteArray (pInputPaths);
    }
    return result;
}
