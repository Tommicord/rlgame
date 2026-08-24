#include "rpack/rpack_encoder.h"
#include "rlgame.base/cstl/cstl_log.h"
#include "rlgame.base/cstl/cstl_string.h"
#include "rlgame.base/cstl/cstl_array.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
R_RPack_PrintHelp (const char* pProgramName)
{
        printf ("\033[1;36mRPACK Texture Packer\033[0m\n");
        printf ("\033[1;36m==================\033[0m\n\n");
        printf ("Usage: %s [OPTIONS] -o OUTPUT.rpack INPUT1 [INPUT2 ...]\n\n", pProgramName);
        printf ("Options:\n");
        printf ("  \033[1;32m-o, --output FILE\033[0m     Output RPACK file path (required)\n");
        printf ("  \033[1;32m-w, --width SIZE\033[0m      Maximum atlas width (default: 4096)\n");
        printf ("  \033[1;32m-h, --height SIZE\033[0m     Maximum atlas height (default: 4096)\n");
        printf ("  \033[1;32m-p, --padding SIZE\033[0m    Padding between textures (default: 1)\n");
        printf ("  \033[1;32m-t, --threshold FLOAT\033[0m  Color similarity threshold (default: 0.1)\n");
        printf ("  \033[1;32m-j, --workers COUNT\033[0m   Number of worker threads (default: 0 = auto)\n");
        printf ("  \033[1;32m-c, --colors\033[0m           Enable colored console output\n");
        printf ("  \033[1;32m--help\033[0m                  Show this help message\n\n");
        printf ("Examples:\n");
        printf ("  %s -o textures.rpack texture1.png texture2.png\n", pProgramName);
        printf ("  %s -o atlas.rpack -w 2048 -h 2048 *.png\n", pProgramName);
        printf ("  %s -o output.rpack -c -t 0.05 image1.png image2.png\n", pProgramName);
        printf ("  %s -o output.rpack -j 4 image1.png image2.png\n\n", pProgramName);
}

static int
R_RPack_ParseArguments (
    int                          argc,
    char**                       argv,
    struct R_RPackEncoderConfig* pConfig,
    char**                       ppOutputPath,
    struct R_CSTL_Array**        ppInputPaths,
    int*                         pEnableColors)
{
        *ppOutputPath = NULL;
        *ppInputPaths = R_CSTL_NewArray ();
        if (!*ppInputPaths)
        {
                return -1;
        }
        *pEnableColors = 0;

        for (int i = 1; i < argc; ++i)
        {
                if (strcmp (argv[i], "--help") == 0 || strcmp (argv[i], "-h") == 0)
                {
                        R_RPack_PrintHelp (argv[0]);
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
                else if (argv[i][0] == '-')
                {
                        fprintf (stderr, "\033[1;31mError: Unknown option '%s'\033[0m\n", argv[i]);
                        return -1;
                }
                else
                {
                        size_t len = strlen (argv[i]) + 1;
                        int result = R_CSTL_ArrayPushData (*ppInputPaths, (const uint8_t*)argv[i], len);
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
R_RPack_LoadRawImage (
    const char*                pPath,
    struct R_RPackInputImage*   pImage,
    uint8_t**                  ppPixelBuffer)
{
        (void)pPath;
        (void)pImage;
        (void)ppPixelBuffer;

        fprintf (
            stderr,
            "\033[1;33mWarning: Image loading from file not yet implemented\033[0m\n");
        fprintf (stderr, "\033[1;33mThis is a placeholder for runtime validation\033[0m\n");

        return -1;
}

static int
R_RPack_InitializeLogging (int enableColors)
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

static struct R_RPackEncoder*
R_RPack_CreateEncoder (const struct R_RPackEncoderConfig* pConfig)
{
        struct R_RPackEncoder* pEncoder = R_RPack_NewEncoder (pConfig);
        if (!pEncoder)
        {
                R_CSTL_LOG_ERROR ("Failed to create encoder");
        }
        return pEncoder;
}

static uint32_t
R_RPack_ProcessInputImages (
    struct R_RPackEncoder* pEncoder,
    const struct R_CSTL_Array* pInputPaths)
{
        uint32_t successCount = 0;
        size_t inputCount = R_CSTL_ArrayLength (pInputPaths) / sizeof(char*);
        size_t offset = 0;
        
        for (size_t i = 0; i < inputCount; ++i)
        {
                char* pPath = NULL;
                R_CSTL_ArrayTypedAt (pInputPaths, char*, offset, &pPath);
                offset += strlen (pPath) + 1;
                
                R_CSTL_LOG_INFO ("Processing input %zu: %s", i + 1, pPath);

                uint8_t* pPixelBuffer = NULL;
                struct R_RPackInputImage image = {0};

                int loadResult = R_RPack_LoadRawImage (pPath, &image, &pPixelBuffer);
                if (loadResult < 0)
                {
                        R_CSTL_LOG_ERROR ("Failed to load image: %s", pPath);
                        continue;
                }

                enum R_RPackError err = R_RPack_EncoderAddImage (pEncoder, &image);
                if (err != R_RPACK_OK)
                {
                        R_CSTL_LOG_ERROR (
                            "Failed to add image '%s': %s",
                            pPath,
                            R_RPackErrorToString (err));
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
R_RPack_EncodeAndWrite (
    struct R_RPackEncoder* pEncoder,
    const char* pOutputPath)
{
        uint64_t requiredSize = R_RPack_EncoderGetRequiredSize (pEncoder);
        R_CSTL_LOG_INFO ("Required output size: %llu bytes", (unsigned long long)requiredSize);

        uint8_t* pOutputBuffer = (uint8_t*)R_CSTL_HeapAlloc (requiredSize);
        if (!pOutputBuffer)
        {
                R_CSTL_LOG_ERROR ("Failed to allocate output buffer");
                return -1;
        }

        uint64_t bytesWritten = 0;
        enum R_RPackError encodeErr =
            R_RPack_EncoderEncode (pEncoder, pOutputBuffer, requiredSize, &bytesWritten);
        if (encodeErr != R_RPACK_OK)
        {
                R_CSTL_LOG_ERROR ("Encoding failed: %s", R_RPackErrorToString (encodeErr));
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
R_RPack_CleanupResources (
    struct R_RPackEncoder* pEncoder,
    struct R_CSTL_Array* pInputPaths)
{
        if (pEncoder)
        {
                R_RPack_DeleteEncoder (pEncoder);
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
        if (argc == 1) {
                R_RPack_PrintHelp (argv[0]);
                return EXIT_SUCCESS;
        }
        
        struct R_RPackEncoderConfig config = {0};
        char*                       pOutputPath = NULL;
        struct R_CSTL_Array*        pInputPaths = NULL;
        int                         enableColors = 0;
        struct R_RPackEncoder*      pEncoder = NULL;
        int                         result = EXIT_FAILURE;

        int parseResult = R_RPack_ParseArguments (
            argc,
            argv,
            &config,
            &pOutputPath,
            &pInputPaths,
            &enableColors);
        if (parseResult == 1)
        {
                return EXIT_SUCCESS;
        }
        if (parseResult < 0)
        {
                return EXIT_FAILURE;
        }
        if (R_RPack_InitializeLogging (enableColors) != 0)
        {
                return EXIT_FAILURE;
        }
        R_CSTL_LOG_INFO ("RPACK: Packing assets");

        pEncoder = R_RPack_CreateEncoder (&config);
        if (!pEncoder)
        {
                goto r_cleanup_logging;
        }
        uint32_t successCount = R_RPack_ProcessInputImages (pEncoder, pInputPaths);
        if (successCount == 0)
        {
                R_CSTL_LOG_ERROR ("No images were successfully processed");
                goto r_cleanup_encoder;
        }
        size_t inputCount = R_CSTL_ArrayLength (pInputPaths) / sizeof(char*);
        R_CSTL_LOG_INFO ("Successfully processed %u/%zu images", successCount, inputCount);

        if (R_RPack_EncodeAndWrite (pEncoder, pOutputPath) != 0)
        {
                goto r_cleanup_encoder;
        }
        R_CSTL_LOG_INFO ("RPACK: Packing completed successfully");
        result = EXIT_SUCCESS;
r_cleanup_encoder:
        if (pEncoder)
        {
                R_RPack_DeleteEncoder (pEncoder);
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
