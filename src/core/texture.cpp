#include "texture.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#define FOURCC_DXT1 0x31545844
#define FOURCC_DXT3 0x33545844
#define FOURCC_DXT5 0x35545844

Texture::~Texture()
{
    if (m_textureID != 0) {
        glDeleteTextures(1, &m_textureID);
        m_textureID = 0;
    }
}

GLuint Texture::loadBMP_custom(const char* imagepath)
{
    printf("Reading image %s\n", imagepath);

    unsigned char header[54];
    unsigned int dataPos = 0;
    unsigned int imageSize = 0;
    unsigned int width = 0;
    unsigned int height = 0;

    FILE* file = fopen(imagepath, "rb");
    if (!file) {
        printf("%s could not be opened.\n", imagepath);
        return 0;
    }

    if (fread(header, 1, 54, file) != 54) {
        printf("Not a correct BMP file\n");
        fclose(file);
        return 0;
    }

    if (header[0] != 'B' || header[1] != 'M') {
        printf("Not a correct BMP file\n");
        fclose(file);
        return 0;
    }

    if (*(int*)&(header[0x1E]) != 0 || *(int*)&(header[0x1C]) != 24) {
        printf("Not a correct BMP file\n");
        fclose(file);
        return 0;
    }

    dataPos = *(unsigned int*)&(header[0x0A]);
    imageSize = *(unsigned int*)&(header[0x22]);
    width = *(unsigned int*)&(header[0x12]);
    height = *(unsigned int*)&(header[0x16]);

    if (imageSize == 0) {
        imageSize = width * height * 3;
    }

    if (dataPos == 0) {
        dataPos = 54;
    }

    if (fseek(file, dataPos, SEEK_SET) != 0) {
        printf("Failed to seek BMP data.\n");
        fclose(file);
        return 0;
    }

    unsigned char* data = new unsigned char[imageSize];

    if (fread(data, 1, imageSize, file) != imageSize) {
        printf("Failed to read BMP image data.\n");
        delete[] data;
        fclose(file);
        return 0;
    }

    fclose(file);

    glGenTextures(1, &m_textureID);
    glBindTexture(GL_TEXTURE_2D, m_textureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
                 GL_BGR, GL_UNSIGNED_BYTE, data);

    delete[] data;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    glGenerateMipmap(GL_TEXTURE_2D);

    return m_textureID;
}

GLuint Texture::loadDDS(const char* imagepath)
{
    unsigned char header[124];

    FILE* fp = fopen(imagepath, "rb");
    if (fp == nullptr) {
        printf("%s could not be opened.\n", imagepath);
        return 0;
    }

    char filecode[4];

    if (fread(filecode, 1, 4, fp) != 4) {
        fclose(fp);
        return 0;
    }

    if (strncmp(filecode, "DDS ", 4) != 0) {
        printf("Not a DDS file: %s\n", imagepath);
        fclose(fp);
        return 0;
    }

    if (fread(header, 1, 124, fp) != 124) {
        printf("Invalid DDS header: %s\n", imagepath);
        fclose(fp);
        return 0;
    }

    unsigned int height = *(unsigned int*)&(header[8]);
    unsigned int width = *(unsigned int*)&(header[12]);
    unsigned int mipMapCount = *(unsigned int*)&(header[24]);
    unsigned int fourCC = *(unsigned int*)&(header[80]);

    if (mipMapCount == 0) {
        mipMapCount = 1;
    }

    unsigned int format = 0;
    unsigned int blockSize = 0;

    switch (fourCC) {
    case FOURCC_DXT1:
        format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        blockSize = 8;
        break;
    case FOURCC_DXT3:
        format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        blockSize = 16;
        break;
    case FOURCC_DXT5:
        format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        blockSize = 16;
        break;
    default:
        printf("Unsupported DDS format. fourCC: %u\n", fourCC);
        fclose(fp);
        return 0;
    }

    unsigned int totalSize = 0;
    unsigned int tempWidth = width;
    unsigned int tempHeight = height;

    for (unsigned int level = 0; level < mipMapCount; ++level) {
        unsigned int size =
            ((tempWidth + 3) / 4) * ((tempHeight + 3) / 4) * blockSize;

        totalSize += size;

        tempWidth = tempWidth > 1 ? tempWidth / 2 : 1;
        tempHeight = tempHeight > 1 ? tempHeight / 2 : 1;
    }

    unsigned char* buffer = static_cast<unsigned char*>(std::malloc(totalSize));

    if (buffer == nullptr) {
        fclose(fp);
        return 0;
    }

    const size_t readBytes = fread(buffer, 1, totalSize, fp);
    fclose(fp);

    if (readBytes != totalSize) {
        printf("DDS read failed. Expected %u bytes, got %zu bytes.\n", totalSize, readBytes);
        std::free(buffer);
        return 0;
    }

    glGenTextures(1, &m_textureID);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    unsigned int offset = 0;
    tempWidth = width;
    tempHeight = height;

    for (unsigned int level = 0; level < mipMapCount; ++level) {
        unsigned int size =
            ((tempWidth + 3) / 4) * ((tempHeight + 3) / 4) * blockSize;

        glCompressedTexImage2D(
            GL_TEXTURE_2D,
            level,
            format,
            tempWidth,
            tempHeight,
            0,
            size,
            buffer + offset
            );

        offset += size;

        tempWidth = tempWidth > 1 ? tempWidth / 2 : 1;
        tempHeight = tempHeight > 1 ? tempHeight / 2 : 1;
    }

    std::free(buffer);

    return m_textureID;
}
GLuint Texture::textureID() const
{
    return m_textureID;
}