//////////////////////////////////////////////////////////////////////////////

#include <memory.h>

static int MemStateCurrentOffset = 0;

void MemStateWrite(void* ptr, size_t size, size_t nmemb, void** stream)
{
    if (stream != NULL)
        memcpy((char*)(*stream) + MemStateCurrentOffset, ptr, size * nmemb);
    MemStateCurrentOffset += size * nmemb;
}

void MemStateWriteOffset(void* ptr, size_t size, size_t nmemb, void** stream, int offset)
{
    if (stream != NULL)
        memcpy((char*)(*stream) + offset, ptr, size * nmemb);
}

int MemStateWriteHeader(void** stream, const char* name, int version)
{
    MemStateWrite((void*)name, 1, 4, stream); // lengths are fixed for this, no need to guess
    MemStateWrite((void*)&version, sizeof(version), 1, stream);
    MemStateWrite((void*)&version, sizeof(version), 1, stream); // place holder for size
    return MemStateCurrentOffset;
}

int MemStateFinishHeader(void** stream, int offset)
{
    int size = 0;
    size = MemStateCurrentOffset - offset;
    MemStateWriteOffset((void*)&size, sizeof(size), 1, stream, offset - 4);
    return (size + 12);
}

void MemStateRead(void* ptr, size_t size, size_t nmemb, const void* stream)
{
    memcpy(ptr, (const char*)stream + MemStateCurrentOffset, size * nmemb);
    MemStateCurrentOffset += size * nmemb;
}

void MemStateReadOffset(void* ptr, size_t size, size_t nmemb, const void* stream, int offset)
{
    memcpy(ptr, (const char*)stream + offset, size * nmemb);
}

int MemStateCheckRetrieveHeader(const void* stream, const char* name, int* version, int* size) {
    char id[4];

    MemStateRead((void*)id, 1, 4, stream);

    if (strncmp(name, id, 4) != 0)
        return -2;

    MemStateRead((void*)version, 1, 4, stream);
    MemStateRead((void*)size, 1, 4, stream);

    return 0;
}

void MemStateSetOffset(int offset) {
    MemStateCurrentOffset = offset;
}

int MemStateGetOffset() {
    return MemStateCurrentOffset;
}

//////////////////////////////////////////////////////////////////////////////