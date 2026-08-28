#include "rpack/rpack_encoder.h"
#include "rpack/rpack_val.h"
#include "rpack/rpack_imgdecode_jpeg.h"

#include "rlgame.base/cstl/cstl_log.h"
#include "rlgame.base/cstl/cstl_string.h"
#include "rlgame.base/cstl/cstl_array.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static int
R_Pack_HasExtension (const char* pPath, const char* pExtension)
{
    size_t pathLength = strlen (pPath);
    size_t extensionLength = strlen (pExtension);
    if (pathLength < extensionLength)
    {
        return 0;
    }
    pPath += pathLength - extensionLength;
    for (size_t i = 0; i < extensionLength; ++i)
    {
        char pathChar = pPath[i];
        char extensionChar = pExtension[i];
        if (pathChar >= 'A' && pathChar <= 'Z') pathChar = (char)(pathChar + ('a' - 'A'));
        if (extensionChar >= 'A' && extensionChar <= 'Z') extensionChar = (char)(extensionChar + ('a' - 'A'));
        if (pathChar != extensionChar) return 0;
    }
    return 1;
}

static void
R_Pack_LogConfigurationWarnings (const struct R_PackEncoderConfig* pConfig)
{
    uint64_t atlasBytes = (uint64_t)pConfig->maxAtlasWidth * pConfig->maxAtlasHeight * 2;
    if (atlasBytes > 32ULL * 1024ULL * 1024ULL)
    {
        R_CSTL_LOG_WARN (
            "Atlas limit is %ux%u (%.1f MiB); large images may exhaust the rpack heap",
            pConfig->maxAtlasWidth,
            pConfig->maxAtlasHeight,
            (double)atlasBytes / (1024.0 * 1024.0));
    }
    if (pConfig->border != 0)
    {
        R_CSTL_LOG_WARN ("Border size %u was requested but border pixels are not encoded yet", pConfig->border);
    }
    if (pConfig->powerOfTwo)
    {
        R_CSTL_LOG_WARN ("Power-of-two atlas sizing was requested but is not implemented yet");
    }
    if (pConfig->enableRotation)
    {
        R_CSTL_LOG_WARN ("Texture rotation was requested but is not implemented yet");
    }
    if (pConfig->alphaThreshold > 0.0f)
    {
        R_CSTL_LOG_WARN ("Alpha threshold %.3f was requested but alpha masking is not implemented yet",
                         pConfig->alphaThreshold);
    }
}

static void
R_Pack_LogImageWarnings (const char* pPath, const struct R_PackInputImage* pImage,
                         const struct R_PackEncoderConfig* pConfig)
{
    uint64_t imageBytes = (uint64_t)pImage->width * pImage->height * 4;
    if (imageBytes > 16ULL * 1024ULL * 1024ULL)
    {
        R_CSTL_LOG_WARN (
            "Image %s is %ux%u (%.1f MiB RGBA); decoding and packing may use significant memory",
            pPath,
            pImage->width,
            pImage->height,
            (double)imageBytes / (1024.0 * 1024.0));
    }
    if (pImage->width > pConfig->maxAtlasWidth || pImage->height > pConfig->maxAtlasHeight)
    {
        R_CSTL_LOG_WARN (
            "Image %s (%ux%u) exceeds the configured atlas limit %ux%u and will be skipped",
            pPath,
            pImage->width,
            pImage->height,
            pConfig->maxAtlasWidth,
            pConfig->maxAtlasHeight);
    }
}

static uint32_t
R_Pack_MipmapDimension (uint32_t source, uint32_t other, uint32_t limit)
{
    uint32_t dimension = source > other ? limit : (uint32_t)(((uint64_t)source * limit) / other);
    return dimension == 0 ? 1 : dimension;
}

static uint8_t*
R_Pack_ResizeImageBox (const struct R_PackInputImage* pSource, uint32_t width, uint32_t height)
{
    uint8_t* pPixels = (uint8_t*)R_CSTL_HeapAlloc ((size_t)width * height * 4);
    if (!pPixels) return NULL;

    for (uint32_t y = 0; y < height; ++y)
    {
        uint32_t sourceY0 = (uint64_t)y * pSource->height / height;
        uint32_t sourceY1 = ((uint64_t)(y + 1) * pSource->height + height - 1) / height;
        if (sourceY1 > pSource->height) sourceY1 = pSource->height;
        for (uint32_t x = 0; x < width; ++x)
        {
            uint32_t sourceX0 = (uint64_t)x * pSource->width / width;
            uint32_t sourceX1 = ((uint64_t)(x + 1) * pSource->width + width - 1) / width;
            if (sourceX1 > pSource->width) sourceX1 = pSource->width;
            uint64_t sums[4] = {0, 0, 0, 0};
            uint32_t count = 0;
            for (uint32_t sourceY = sourceY0; sourceY < sourceY1; ++sourceY)
                for (uint32_t sourceX = sourceX0; sourceX < sourceX1; ++sourceX)
                {
                    const uint8_t* pSourcePixel = pSource->pPixels + (size_t)sourceY * pSource->stride + sourceX * 4;
                    for (uint32_t channel = 0; channel < 4; ++channel) sums[channel] += pSourcePixel[channel];
                    ++count;
                }
            uint8_t* pDestinationPixel = pPixels + ((size_t)y * width + x) * 4;
            for (uint32_t channel = 0; channel < 4; ++channel) pDestinationPixel[channel] = (uint8_t)(sums[channel] / count);
        }
    }
    return pPixels;
}

static int
R_Pack_MakeVariantPath (const char* pOutputPath, uint32_t size, char** ppVariantPath)
{
    const char* pExtension = strrchr (pOutputPath, '.');
    size_t stemLength = pExtension ? (size_t)(pExtension - pOutputPath) : strlen (pOutputPath);
    size_t suffixLength = strlen ("_4294967295x4294967295.rpack");
    char* pPath = (char*)R_CSTL_HeapAlloc (stemLength + suffixLength + 1);
    if (!pPath) return -1;
    snprintf (pPath, stemLength + suffixLength + 1, "%.*s_%ux%u.rpack", (int)stemLength, pOutputPath, size, size);
    *ppVariantPath = pPath;
    return 0;
}

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
    printf ("  -H, --height SIZE           = Maximum atlas height in pixels (default: 4096).\n");
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
    printf ("  --mipmap                    = Generate 64x64 through 1x1 RPACK variants.\n");
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
    int*                        pQuiet,
    int*                        pMipmap)
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
    *pMipmap = 0;

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
        else if (strcmp (argv[i], "-H") == 0 || strcmp (argv[i], "--height") == 0)
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

static int
R_Pack_LoadImgAsset (const char* pPath, struct R_PackInputImage* pImage, uint8_t** ppPixelBuffer)
{
    if (!pPath || !pImage || !ppPixelBuffer)
    {
        return -1;
    }
    *ppPixelBuffer = NULL;
    memset (pImage, 0, sizeof (*pImage));

    if (!R_Pack_HasExtension (pPath, ".jpg") && !R_Pack_HasExtension (pPath, ".jpeg"))
    {
        R_CSTL_LOG_WARN ("Skipping unsupported image format: %s (JPEG expected)", pPath);
        return -1;
    }

    struct R_PackJpegImage decoded = {0};
    enum R_PackError       error = R_Pack_JpegDecodeFile (pPath, &decoded);
    if (error != R_RPACK_OK)
    {
        fprintf (stderr, "JPEG decode failed for %s: %s\n", pPath, R_Pack_ErrorToString (error));
        return -1;
    }

    *ppPixelBuffer = decoded.pPixels;
    pImage->pPixels = decoded.pPixels;
    pImage->width = decoded.width;
    pImage->height = decoded.height;
    pImage->stride = decoded.stride;
    pImage->pName = pPath;
    return 0;
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
R_Pack_EncodeInputImages (struct R_PackEncoder* pEncoder, const struct R_CSTL_Array* pInputPaths, uint32_t mipmapSize)
{
    uint32_t    successCount = 0;
    size_t      inputCount = 0;
    size_t      offset = 0;
    size_t      inputBytes = R_CSTL_ArrayLength (pInputPaths);
    const char* pInputData = (const char*)R_CSTL_ArrayData (pInputPaths);

    while (offset < inputBytes)
    {
        size_t pathLength = strnlen (pInputData + offset, inputBytes - offset);
        if (pathLength == inputBytes - offset) break;
        ++inputCount;
        offset += pathLength + 1;
    }
    offset = 0;

    for (size_t i = 0; i < inputCount; ++i)
    {
        char* pPath = (char*)(pInputData + offset);
        offset += strlen (pPath) + 1;

        R_CSTL_LOG_INFO ("Processing input %zu: %s", i + 1, pPath);
        uint8_t*                pPixelBuffer = NULL;
        struct R_PackInputImage image = {0};

        int loadResult = R_Pack_LoadImgAsset (pPath, &image, &pPixelBuffer);
        if (loadResult < 0)
        {
            R_CSTL_LOG_WARN ("Skipping image after load failure: %s", pPath);
            continue;
        }

        if (mipmapSize != 0 && (image.width > mipmapSize || image.height > mipmapSize))
        {
            uint32_t width = R_Pack_MipmapDimension (image.width, image.height, mipmapSize);
            uint32_t height = R_Pack_MipmapDimension (image.height, image.width, mipmapSize);
            uint8_t* pResizedPixels = R_Pack_ResizeImageBox (&image, width, height);
            if (!pResizedPixels)
            {
                R_CSTL_LOG_WARN ("Skipping mipmap level for %s: resize allocation failed", pPath);
                R_CSTL_HeapFree (pPixelBuffer);
                continue;
            }
            R_CSTL_HeapFree (pPixelBuffer);
            pPixelBuffer = pResizedPixels;
            image.pPixels = pResizedPixels;
            image.width = width;
            image.height = height;
            image.stride = width * 4;
        }

        R_Pack_LogImageWarnings (pPath, &image, &pEncoder->config);

        enum R_PackError err = R_Pack_EncoderAddImage (pEncoder, &image);
        if (err != R_RPACK_OK)
        {
            R_CSTL_LOG_WARN ("Skipping image '%s': %s", pPath, R_Pack_ErrorToString (err));
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

    if (requiredSize > 32ULL * 1024ULL * 1024ULL)
    {
        R_CSTL_LOG_WARN ("Output requires %.1f MiB; the CLI heap is limited to 64 MiB",
                         (double)requiredSize / (1024.0 * 1024.0));
    }

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
        R_CSTL_LOG_ERROR ("Encoding failed: %s", R_Pack_ErrorToString (encodeErr));
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

    struct R_PackValidationReport report;
    if (!R_Pack_ValidatePackedData (pOutputBuffer, bytesWritten, &report))
    {
        R_CSTL_LOG_ERROR (
            "RPACK validation failed: %s at byte %llu (texture %u, pixel %llu)",
            R_Pack_ErrorToString (report.error),
            (unsigned long long)report.offset,
            report.textureIndex,
            (unsigned long long)report.pixelIndex);
        remove (pOutputPath);
        R_CSTL_HeapFree (pOutputBuffer);
        return -1;
    }

    R_CSTL_LOG_INFO ("RPACK validation passed: %llu bytes verified", (unsigned long long)bytesWritten);

    R_CSTL_LOG_INFO ("Successfully wrote %llu bytes to %s", (unsigned long long)bytesWritten, pOutputPath);
    R_CSTL_HeapFree (pOutputBuffer);
    return 0;
}

static int
R_Pack_EncodeMipmapVariants (const struct R_PackEncoderConfig* pConfig, const struct R_CSTL_Array* pInputPaths,
                             const char* pOutputPath)
{
    static const uint32_t mipmapSizes[] = {64, 32, 16, 8, 4, 2, 1};
    size_t generated = 0;
    for (size_t i = 0; i < sizeof (mipmapSizes) / sizeof (mipmapSizes[0]); ++i)
    {
        char* pVariantPath = NULL;
        if (R_Pack_MakeVariantPath (pOutputPath, mipmapSizes[i], &pVariantPath) != 0)
        {
            R_CSTL_LOG_ERROR ("Failed to allocate mipmap output path for %ux%u", mipmapSizes[i], mipmapSizes[i]);
            return -1;
        }

        struct R_PackEncoder* pVariantEncoder = R_Pack_CreateEncoder (pConfig);
        if (!pVariantEncoder)
        {
            R_CSTL_LOG_ERROR ("Failed to create encoder for mipmap level %ux%u", mipmapSizes[i], mipmapSizes[i]);
            R_CSTL_HeapFree (pVariantPath);
            return -1;
        }

        uint32_t successCount = R_Pack_EncodeInputImages (pVariantEncoder, pInputPaths, mipmapSizes[i]);
        if (successCount == 0 || R_Pack_EncodeAndWrite (pVariantEncoder, pVariantPath) != 0)
        {
            R_CSTL_LOG_WARN ("Mipmap level %ux%u was not generated", mipmapSizes[i], mipmapSizes[i]);
            R_Pack_DeleteEncoder (pVariantEncoder);
            R_CSTL_HeapFree (pVariantPath);
            continue;
        }
        R_CSTL_LOG_INFO ("Generated mipmap variant %s (%u images)", pVariantPath, successCount);
        ++generated;
        R_Pack_DeleteEncoder (pVariantEncoder);
        R_CSTL_HeapFree (pVariantPath);
    }
    return generated == 0 ? -1 : 0;
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
    if (R_CSTL_HeapInit (64 * 1024 * 1024) != R_CSTL_OK)
    {
        fprintf (stderr, "Error: failed to initialize rpack heap\n");
        return EXIT_FAILURE;
    }
    if (argc == 1)
    {
        R_Pack_PrintHelp ();
        R_CSTL_HeapShutdown ();
        return EXIT_SUCCESS;
    }

    struct R_PackEncoderConfig config = {0};
    char*                      pOutputPath = NULL;
    struct R_CSTL_Array*       pInputPaths = NULL;
    int                        enableColors = 0;
    struct R_PackEncoder*      pEncoder = NULL;
    int                        verbose = 0;
    int                        quiet = 0;
    int                        mipmap = 0;
    int                        result = EXIT_FAILURE;

    int parseResult = R_Pack_ParseArguments (
        argc,
        argv,
        &config,
        &pOutputPath,
        &pInputPaths,
        &enableColors,
        &verbose,
        &quiet,
        &mipmap);
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
    if (R_Pack_InitializeLogging (enableColors) != 0)
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
    R_CSTL_LOG_INFO ("Packing assets");
    R_Pack_LogConfigurationWarnings (&config);

    pEncoder = R_Pack_CreateEncoder (&config);
    if (!pEncoder)
    {
        goto r_cleanup_logging;
    }
    uint32_t successCount = R_Pack_EncodeInputImages (pEncoder, pInputPaths, 0);
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
    R_CSTL_HeapShutdown ();
    return result;
}
