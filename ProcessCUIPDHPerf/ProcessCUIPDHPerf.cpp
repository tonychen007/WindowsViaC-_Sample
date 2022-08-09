#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>
#include <stdio.h>
#include <strsafe.h>

#pragma comment(lib, "advapi32.lib")

#define INIT_OBJECT_BUFFER_SIZE 48928   // Initial buffer size to use when querying specific objects.
#define INIT_GLOBAL_BUFFER_SIZE 122880  // Initial buffer size to use when using "Global" to query all objects.
#define BUFFER_INCREMENT 122880          // Increment buffer size in 16K chunks.
#define MAX_INSTANCE_NAME_LEN      255  // Max length of an instance name.
#define MAX_FULL_INSTANCE_NAME_LEN 511  // Form is parentinstancename/instancename#nnn.

/* 
*   -----------------       -------------------------       --------------------------
    |PERF_DATA_BLOCK|    /  |   PERF_OBJECT_TYPE    |      |PERF_COUNTER_DEFINITION |
    -----------------   /    -------------------------       --------------------------
    | First Object  | -/    |PERF_COUNTER_DEFINITION|       |PERF_INSTANCE_DEFINITION|
    -----------------       -------------------------       --------------------------
    | Second Object |       |PERF_COUNTER_DEFINITION|       | Name of first Instance |
    -----------------       -------------------------       --------------------------
    |...............|       |PERF_COUNTER_DEFINITION|       |  PERF_COUNTER_BLOCK    |
    -----------------       -------------------------       --------------------------
    | Last Object   |       |  PERF_COUNTER_BLOCK   |       |PERF_INSTANCE_DEFINITION|
    -----------------       -------------------------       --------------------------
                                                            | Name of second Instance|
                                                            ---------------------------
                                                            |  PERF_COUNTER_BLOCK    |                          |                                   

*/



//Identifies a performance object and its counters and instances.
typedef struct _perf_object
{
    DWORD dwObjectIndex;          // Index to the name of the object.
    LPDWORD pdwCounterIndexes;    // Array of counter index values.
    DWORD dwNumberOfCounters;
    LPWSTR pInstanceNames;        // Array of instance name strings.
    DWORD dwNumberofInstances;
} PERF_OBJECT, * PPERF_OBJECT;



LPBYTE GetPerformanceData(LPWSTR pwszSource, DWORD dwInitialBufferSize);
BOOL GetCounterTextStrings(LPWSTR& pCounterTextHead, LPWSTR& pHelpTextHead);
LPWSTR GetText(LPWSTR pwszSource);
BOOL BuildTextTable(LPWSTR pCounterHead, LPWSTR pHelpHead, LPDWORD* pOffsetsHead, LPDWORD pNumberOfOffsets);
DWORD GetNumberOfTextEntries(LPWSTR pwszSource);
PPERF_OBJECT LoadObjects(LPBYTE pObject, DWORD* dwNumberOfObjects, LPBYTE pEndObject);
BOOL LoadCounters(LPDWORD* ppdwCounterIndexes, LPBYTE pCounter, DWORD* pdwNumberOfCounters);
BOOL LoadInstances(LPWSTR* pInstanceNames, LPBYTE pInstance, DWORD dwNumberofInstances, DWORD CodePage);
BOOL GetFullInstanceName(PERF_INSTANCE_DEFINITION* pInstance, DWORD CodePage, WCHAR* pName);
BOOL ConvertNameToUnicode(UINT CodePage, LPCSTR pNameToConvert, DWORD dwNameToConvertLen, LPWSTR pConvertedName);
PERF_INSTANCE_DEFINITION* GetParentInstance(PERF_OBJECT_TYPE* pObject, DWORD dwInstancePosition);
PERF_OBJECT_TYPE* GetObject(DWORD dwObjectToFind);
//int CompareObjects(const void* pObject1, const void* pObject2);
//int CompareCounters(const void* pCounter1, const void* pCounter2);
//int CompareInstances(const void* pInstance1, const void* pInstance2);
void PrintObjectNames(DWORD dwNumberOfObjects, BOOL fIncludeCounters, BOOL fIncludeInstances);
void FreePerfObjects(PPERF_OBJECT pObjects, DWORD dwNumberOfObjects);

// Globals
LPBYTE g_pPerfDataHead = NULL;    // Head of the performance data.
LPWSTR g_pCounterTextHead = NULL; // Head of the MULTI_SZ buffer that contains the Counter text.
LPWSTR g_pHelpTextHead = NULL;    // Head of the MULTI_SZ buffer that contains the Help text.
LPDWORD g_pTextOffsets = NULL;    // Array of DWORDS that contain the offsets to the text in
                                  // pCounterTextHead and pHelpTextHead. The text index
                                  // values mirror the array index.
PPERF_OBJECT g_pObjects = NULL;   // Array of PERF_OBJECTs.


void wmain(void) {
    DWORD dwNumberOfOffsets = 0;    // Number of elements in the pTextOffsets array.
    DWORD dwNumberOfObjects = 0;    // Number of elements in the pObjects array.
    PERF_DATA_BLOCK* pPerfBlock;

    g_pPerfDataHead = (LPBYTE)GetPerformanceData((LPWSTR)L"Global", INIT_GLOBAL_BUFFER_SIZE);
    pPerfBlock = (PERF_DATA_BLOCK*)g_pPerfDataHead;

    GetCounterTextStrings(g_pCounterTextHead, g_pHelpTextHead);
    BuildTextTable(g_pCounterTextHead, g_pHelpTextHead, &g_pTextOffsets, &dwNumberOfOffsets);
    dwNumberOfObjects = pPerfBlock->NumObjectTypes;
    g_pObjects = LoadObjects(g_pPerfDataHead + pPerfBlock->HeaderLength, &dwNumberOfObjects, g_pPerfDataHead + pPerfBlock->TotalByteLength);

    PrintObjectNames(dwNumberOfObjects, TRUE, TRUE);
}

LPBYTE GetPerformanceData(LPWSTR pwszSource, DWORD dwInitialBufferSize) {
    LPBYTE pBuffer = NULL;
    DWORD dwBufferSize = 0;        //Size of buffer, used to increment buffer size
    DWORD dwSize = 0;              //Size of buffer, used when calling RegQueryValueEx
    LPBYTE pTemp = NULL;           //Temp variable for realloc() in case it fails
    LONG status = ERROR_SUCCESS;

    dwBufferSize = dwSize = dwInitialBufferSize;
    pBuffer = (LPBYTE)malloc(dwBufferSize);

    while (ERROR_MORE_DATA == (status = RegQueryValueEx(HKEY_PERFORMANCE_DATA, pwszSource, NULL, NULL, pBuffer, &dwSize)))     {
        dwBufferSize += BUFFER_INCREMENT;
        pTemp = (LPBYTE)realloc(pBuffer, dwBufferSize);
        pBuffer = pTemp;
        dwSize = dwBufferSize;
    }

    RegCloseKey(HKEY_PERFORMANCE_DATA);
    return pBuffer;
}


BOOL GetCounterTextStrings(LPWSTR& pCounterTextHead, LPWSTR& pHelpTextHead) {
    BOOL status = TRUE;
    pCounterTextHead = GetText((LPWSTR)L"Counter");
    pHelpTextHead = GetText((LPWSTR)L"Help");
    return status;
}


// Get the text based on the source value. This function uses the
// HKEY_PERFORMANCE_NLSTEXT key to get the strings.
LPWSTR GetText(LPWSTR pwszSource) {
    LPWSTR pBuffer = NULL;
    DWORD dwBufferSize = 0;
    LONG status = ERROR_SUCCESS;

    // Query the size of the text data so you can allocate the buffer.
    status = RegQueryValueEx(HKEY_PERFORMANCE_NLSTEXT, pwszSource, NULL, NULL, NULL, &dwBufferSize);

    // Allocate the text buffer and query the text.
    pBuffer = (LPWSTR)malloc(dwBufferSize);
    status = RegQueryValueEx(HKEY_PERFORMANCE_NLSTEXT, pwszSource, NULL, NULL, (LPBYTE)pBuffer, &dwBufferSize);

    return pBuffer;
}


// Build a table of offsets into the counter and help text buffers. Use the index
// values from the performance data queries to access the offsets.
BOOL BuildTextTable(LPWSTR pCounterHead, LPWSTR pHelpHead, LPDWORD* pOffsetsHead, LPDWORD pNumberOfOffsets) {
    BOOL fSuccess = FALSE;
    LPWSTR pwszCounterText = NULL;  // Used to cycle through the Counter text
    LPWSTR pwszHelpText = NULL;     // Used to cycle through the Help text
    LPDWORD pOffsets = NULL;
    DWORD dwCounterIndex = 0;       // Index value of the Counter text
    DWORD dwHelpIndex = 0;          // Index value of the Help text
    DWORD dwSize = 0;               // Size of the block of memory that holds the offset array

    pwszCounterText = pCounterHead;
    pwszHelpText = pHelpHead;

    *pNumberOfOffsets = GetNumberOfTextEntries((LPWSTR)L"Last Help");
    dwSize = sizeof(DWORD) * (*pNumberOfOffsets + 1);  // Add one to make the array one-based
    pOffsets = (LPDWORD)malloc(dwSize);

    ZeroMemory(pOffsets, dwSize);
    *pOffsetsHead = pOffsets;

    // Bypass first record (pair) of the counter data - contains upper bounds of system counter index.
    pwszCounterText += (wcslen(pwszCounterText) + 1);
    pwszCounterText += (wcslen(pwszCounterText) + 1);

    for (; *pwszCounterText; pwszCounterText += (wcslen(pwszCounterText) + 1)) {
        dwCounterIndex = _wtoi(pwszCounterText);
        dwHelpIndex = _wtoi(pwszHelpText);

        // Use the counter's index value as an indexer into the pOffsets array.
        // Store the offset to the counter text in the array element.
        pwszCounterText += (wcslen(pwszCounterText) + 1);  //Skip past index value
        pOffsets[dwCounterIndex] = (DWORD)(pwszCounterText - pCounterHead);

        // Some help indexes for system counters do not have a matching counter, so loop
        // until you find the matching help index or the index is greater than the corresponding
        // counter index. For example, if the indexes were as follows, you would loop
        // until the help index was 11.
        //
        // Counter index       Help Index
        //   2                    3
        //   4                    5
        //   6                    7
        //                        9   (skip because there is no matching Counter index)
        //   10                   11
        while (*pwszHelpText && dwHelpIndex < dwCounterIndex)
        {
            pwszHelpText += (wcslen(pwszHelpText) + 1);  // Skip past index value
            pwszHelpText += (wcslen(pwszHelpText) + 1);  // Skip past help text to the next index value
            dwHelpIndex = _wtoi(pwszHelpText);
        }

        // Use the Help index value as an indexer into the pOffsets array.
        // Store the offset to the help text in the array element.
        if (dwHelpIndex == (dwCounterIndex + 1))
        {
            pwszHelpText += (wcslen(pwszHelpText) + 1);  //Skip past index value
            pOffsets[dwHelpIndex] = (DWORD)(pwszHelpText - pHelpHead);
            pwszHelpText += (wcslen(pwszHelpText) + 1);  //Skip past help text to next index value
        }
    }

    fSuccess = TRUE;

    return fSuccess;
}


// Retrieve the last help index used from the registry. Use this number
// to allocate an array of DWORDs. Note that index values are not contiguous.
DWORD GetNumberOfTextEntries(LPWSTR pwszSource) {
    DWORD dwEntries = 0;
    LONG status = ERROR_SUCCESS;
    HKEY hkey = NULL;
    DWORD dwSize = sizeof(DWORD);
    LPWSTR pwszMessage = NULL;

    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib",
        0,
        KEY_READ,
        &hkey);

    status = RegQueryValueEx(hkey, pwszSource, NULL, 0, (LPBYTE)&dwEntries, &dwSize);

    if (hkey)
        RegCloseKey(hkey);

    return dwEntries;
}


// Allocate a block of memory that will contain one or more PERF_OBJECT structures.
// For each object, also load its counters and instances.

PPERF_OBJECT LoadObjects(LPBYTE pPerfDataObject, DWORD* pdwNumberOfObjects, LPBYTE pEndObject) {
    BOOL fSuccess = FALSE;
    DWORD dwSize = sizeof(PERF_OBJECT) * *pdwNumberOfObjects;
    PPERF_OBJECT pObjects = NULL;
    DWORD i = 0;

    pObjects = (PERF_OBJECT*)malloc(dwSize);
    PERF_OBJECT_TYPE* pObjectType = (PERF_OBJECT_TYPE*)pPerfDataObject;

    ZeroMemory(pObjects, dwSize);
    for (; i < *pdwNumberOfObjects; i++) {
        pObjects[i].dwObjectIndex = pObjectType->ObjectNameTitleIndex;
        // Load Counter indexes.

        pObjects[i].dwNumberOfCounters = pObjectType->NumCounters;

        fSuccess = LoadCounters(&(pObjects[i].pdwCounterIndexes),
            pPerfDataObject + pObjectType->HeaderLength,  // Points to the first counter
            &(pObjects[i].dwNumberOfCounters));

        if (pObjectType->NumInstances != 0 &&
            pObjectType->NumInstances != PERF_NO_INSTANCES) {

            // Load instance indexes.

            pObjects[i].dwNumberofInstances = pObjectType->NumInstances;
            fSuccess = LoadInstances(&(pObjects[i].pInstanceNames),
                pPerfDataObject + pObjectType->DefinitionLength, // Points to the first instance
                pObjectType->NumInstances,
                pObjectType->CodePage);
        }

        pPerfDataObject += pObjectType->TotalByteLength;
        if (pPerfDataObject >= pEndObject) {
            // The end of the counter objects array was reached.
            break;
        }
        pObjectType = (PERF_OBJECT_TYPE*)pPerfDataObject;
    }

    *pdwNumberOfObjects = i;

    //After all the data is loaded, sort the list of objects, so you can print
    //them in alphabetical order.

    //qsort(pObjects, *pdwNumberOfObjects, sizeof(PERF_OBJECT), CompareObjects);

cleanup:

    if (FALSE == fSuccess)
    {
        FreePerfObjects(pObjects, i);
        pObjects = NULL;
    }

    return pObjects;
}


// Allocate of block of memory and load the index values for the counters that
// the object defines.
BOOL LoadCounters(LPDWORD* ppdwIndexes, LPBYTE pCounter, DWORD* pdwNumberOfCounters) {
    BOOL fSuccess = FALSE;
    DWORD dwSize = sizeof(DWORD) * *pdwNumberOfCounters;
    LPDWORD pIndexes = NULL;
    DWORD dwSkippedBaseRecords = 0;
    PERF_COUNTER_DEFINITION* pCounterDef = (PERF_COUNTER_DEFINITION*)pCounter;

    pIndexes = (LPDWORD)malloc(dwSize);
    ZeroMemory(pIndexes, dwSize);

    for (DWORD i = 0, j = 0; i < *pdwNumberOfCounters; i++) {
        // Ignore base counters.
        if ( (pCounterDef->CounterType & PERF_COUNTER_BASE) == PERF_COUNTER_BASE) {
            dwSkippedBaseRecords++;
        }
        else {
            pIndexes[j] = pCounterDef->CounterNameTitleIndex;
            j++;
        }

        pCounter += pCounterDef->ByteLength;
        pCounterDef = (PERF_COUNTER_DEFINITION*)pCounter;
    }

    // Decrement so the sort works.
    *pdwNumberOfCounters -= dwSkippedBaseRecords;

    // Sort the index values of the object's counters.
    //qsort(pIndexes, *pdwNumberOfCounters, sizeof(DWORD), CompareCounters);
    *ppdwIndexes = pIndexes;
    fSuccess = TRUE;

cleanup:

    return fSuccess;
}

// Allocate a block of memory and load the full instance name for each instance
// of the object.
BOOL LoadInstances(LPWSTR* ppNames, LPBYTE pInstance, DWORD dwNumberofInstances, DWORD CodePage) {
    BOOL fSuccess = FALSE;
    DWORD dwSize = (sizeof(WCHAR) * (MAX_FULL_INSTANCE_NAME_LEN + 1)) * dwNumberofInstances;
    PERF_COUNTER_BLOCK* pCounter = NULL;
    LPWSTR pName = NULL;
    PERF_INSTANCE_DEFINITION* pInstanceDef = (PERF_INSTANCE_DEFINITION*)pInstance;

    *ppNames = (LPWSTR)malloc(dwSize);
    ZeroMemory(*ppNames, dwSize);
    pName = *ppNames;

    for (DWORD i = 0; i < dwNumberofInstances; i++) {
        fSuccess = GetFullInstanceName(pInstanceDef, CodePage, pName);
        pName = pName + (MAX_FULL_INSTANCE_NAME_LEN + 1);

        pCounter = (PERF_COUNTER_BLOCK*)(pInstance + pInstanceDef->ByteLength);
        pInstance += pInstanceDef->ByteLength + pCounter->ByteLength;
        pInstanceDef = (PERF_INSTANCE_DEFINITION*)pInstance;
    }

    // Sort the instance names of the object's instances.
    //qsort(*ppNames, dwNumberofInstances, sizeof(WCHAR) * (MAX_FULL_INSTANCE_NAME_LEN + 1), CompareInstances);

cleanup:

    return fSuccess;
}


// Retrieve the full name of the instance. The full name of the instance includes
// the name of this instance and its parent instance, if this instance is a
// child instance. The full name is in the form, "parent name/child name".
// For example, a thread instance is a child of a process instance.
//
// Providers are encouraged to use Unicode strings for instance names. If
// PERF_INSTANCE_DEFINITION.CodePage is zero, the name is in Unicode; otherwise,
// use the CodePage value to convert the string to Unicode.
BOOL GetFullInstanceName(PERF_INSTANCE_DEFINITION* pInstance, DWORD CodePage, WCHAR* pName) {
    BOOL fSuccess = TRUE;
    PERF_INSTANCE_DEFINITION* pParentInstance = NULL;
    PERF_OBJECT_TYPE* pParentObject = NULL;
    DWORD dwLength = 0;
    WCHAR wszInstanceName[MAX_INSTANCE_NAME_LEN + 1];
    WCHAR wszParentInstanceName[MAX_INSTANCE_NAME_LEN + 1];

    if (CodePage == 0) { // Instance name is a Unicode string
        // PERF_INSTANCE_DEFINITION->NameLength is in bytes, so convert to characters.
        dwLength = (MAX_INSTANCE_NAME_LEN < (pInstance->NameLength / 2)) ? MAX_INSTANCE_NAME_LEN : pInstance->NameLength / 2;
        StringCchCopyN(wszInstanceName, MAX_INSTANCE_NAME_LEN + 1, (LPWSTR)(((LPBYTE)pInstance) + pInstance->NameOffset), dwLength);
        wszInstanceName[dwLength] = '\0';
    }
    else { // Convert the multi-byte instance name to Unicode
        fSuccess = ConvertNameToUnicode(CodePage,
            (LPCSTR)(((LPBYTE)pInstance) + pInstance->NameOffset),  // Points to string
            pInstance->NameLength,
            wszInstanceName);

        if (FALSE == fSuccess) {
            wprintf(L"ConvertNameToUnicode for instance failed.\n");
            goto cleanup;
        }
    }

    if (pInstance->ParentObjectTitleIndex) {
        // Use the index to find the parent object. The pInstance->ParentObjectInstance
        // member tells you that the parent instance is the nth instance of the
        // parent object.
        pParentObject = GetObject(pInstance->ParentObjectTitleIndex);
        pParentInstance = GetParentInstance(pParentObject, pInstance->ParentObjectInstance);

        if (CodePage == 0) { // Instance name is a Unicode string
            dwLength = (MAX_INSTANCE_NAME_LEN < pParentInstance->NameLength / 2) ? MAX_INSTANCE_NAME_LEN : pParentInstance->NameLength / 2;
            StringCchCopyN(wszParentInstanceName, MAX_INSTANCE_NAME_LEN + 1, (LPWSTR)(((LPBYTE)pParentInstance) + pParentInstance->NameOffset), dwLength);
            wszParentInstanceName[dwLength] = '\0';
        }
        else {  // Convert the multi-byte instance name to Unicode
            fSuccess = ConvertNameToUnicode(CodePage,
                (LPCSTR)(((LPBYTE)pParentInstance) + pParentInstance->NameOffset),  //Points to string.
                pInstance->NameLength,
                wszParentInstanceName);

            if (FALSE == fSuccess) {
                wprintf(L"ConvertNameToUnicode for parent instance failed.\n");
                goto cleanup;
            }
        }

        StringCchPrintf(pName, MAX_FULL_INSTANCE_NAME_LEN + 1, L"%s/%s", wszParentInstanceName, wszInstanceName);
    }
    else {
        StringCchPrintf(pName, MAX_INSTANCE_NAME_LEN + 1, L"%s", wszInstanceName);
    }

cleanup:

    return fSuccess;
}


// Converts a multi-byte string to a Unicode string. If the input string is longer than
// MAX_INSTANCE_NAME_LEN, the input string is truncated.
BOOL ConvertNameToUnicode(UINT CodePage, LPCSTR pNameToConvert, DWORD dwNameToConvertLen, LPWSTR pConvertedName)
{
    BOOL fSuccess = FALSE;
    int CharsConverted = 0;
    DWORD dwLength = 0;

    // dwNameToConvertLen is in bytes, so convert MAX_INSTANCE_NAME_LEN to bytes.
    dwLength = (MAX_INSTANCE_NAME_LEN * sizeof(WCHAR) < (dwNameToConvertLen)) ? MAX_INSTANCE_NAME_LEN * sizeof(WCHAR) : dwNameToConvertLen;

    CharsConverted = MultiByteToWideChar((UINT)CodePage, 0, pNameToConvert, dwLength, pConvertedName, MAX_INSTANCE_NAME_LEN);
    if (CharsConverted)
    {
        pConvertedName[dwLength] = '\0';
        fSuccess = TRUE;
    }
    else
    {
        // If the specified code page didn't work, try one more time, assuming that the input string is Unicode.
        dwLength = (MAX_INSTANCE_NAME_LEN < (dwNameToConvertLen / 2)) ? MAX_INSTANCE_NAME_LEN : dwNameToConvertLen / 2;
        if (SUCCEEDED(StringCchCopyN(pConvertedName, MAX_INSTANCE_NAME_LEN + 1, (LPWSTR)pNameToConvert, dwLength)))
        {
            pConvertedName[dwLength] = '\0';
            fSuccess = TRUE;
        }
    }

    return fSuccess;
}


// Find the object using the specified index value.
PERF_OBJECT_TYPE* GetObject(DWORD dwObjectToFind)
{
    LPBYTE pObject = g_pPerfDataHead + ((PERF_DATA_BLOCK*)g_pPerfDataHead)->HeaderLength;
    DWORD dwNumberOfObjects = ((PERF_DATA_BLOCK*)g_pPerfDataHead)->NumObjectTypes;
    BOOL fFoundObject = FALSE;
    PERF_OBJECT_TYPE* pObjType = (PERF_OBJECT_TYPE*)pObject;

    for (DWORD i = 0; i < dwNumberOfObjects; i++) {
        if (dwObjectToFind == pObjType->ObjectNameTitleIndex) {
            fFoundObject = TRUE;
            break;
        }

        pObject += pObjType->TotalByteLength;
        pObjType = (PERF_OBJECT_TYPE*)pObject;
    }

    return (fFoundObject) ? pObjType : NULL;
}


// Find the nth instance of an object.
PERF_INSTANCE_DEFINITION* GetParentInstance(PERF_OBJECT_TYPE* pObject, DWORD dwInstancePosition)
{
    LPBYTE pInstance = (LPBYTE)pObject + pObject->DefinitionLength;
    PERF_COUNTER_BLOCK* pCounter = NULL;
    PERF_INSTANCE_DEFINITION* pPerfInstance = (PERF_INSTANCE_DEFINITION*)pInstance;

    for (DWORD i = 0; i < dwInstancePosition; i++)
    {
        pCounter = (PERF_COUNTER_BLOCK*)(pInstance + pPerfInstance->ByteLength);
        pInstance += pPerfInstance->ByteLength + pCounter->ByteLength;
        pPerfInstance = (PERF_INSTANCE_DEFINITION*)pInstance;
    }

    return (PERF_INSTANCE_DEFINITION*)pInstance;
}

// Used by qsort to put the instance names in alphabetical order.
int CompareInstances(const void* pName1, const void* pName2)
{
    return _wcsicmp((LPWSTR)pName1, (LPWSTR)pName2);
}


// Print the object names and optionally print their counter and instance names.
void PrintObjectNames(DWORD dwNumberOfObjects, BOOL fIncludeCounters, BOOL fIncludeInstances)
{
    PERF_INSTANCE_DEFINITION* pInstance = NULL;
    DWORD dwIncrementSize = sizeof(WCHAR) * (MAX_FULL_INSTANCE_NAME_LEN + 1);
    DWORD index = 0;
    DWORD dwSerialNo = 0;

    for (DWORD i = 0; i < dwNumberOfObjects; i++)
    {
        index = g_pObjects[i].dwObjectIndex;
        wprintf(L"%d %s\n", index, g_pCounterTextHead + g_pTextOffsets[index]);

        // There can be multiple instances with duplicate names. For example, the
        // Process object can have multiple instance of svchost. To differentiate
        // the instances, append a serial number to the name of duplicate instances
        // If the object contained three svchost names, the function prints them
        // as svchost, svchost#1, and svchost#2.
        // If running on Windows 10 20H2 or later, you can avoid this issue by
        // using the "Process V2" counterset.
        if (fIncludeInstances && g_pObjects[i].dwNumberofInstances > 0) {
            dwSerialNo = 0;

            for (DWORD j = 0; j < g_pObjects[i].dwNumberofInstances; j++) {
                LPWSTR pName1 = g_pObjects[i].pInstanceNames + ((j - 1) * (MAX_FULL_INSTANCE_NAME_LEN + 1));
                if (j > 0) {
                    LPWSTR pName2 = g_pObjects[i].pInstanceNames + (j * (MAX_FULL_INSTANCE_NAME_LEN + 1));
                    if (!CompareInstances(pName1, pName2)) {
                        dwSerialNo++;
                    }
                    else {
                        dwSerialNo = 0;
                    }
                }
                pName1 = g_pObjects[i].pInstanceNames + ((j) * (MAX_FULL_INSTANCE_NAME_LEN + 1));
                if (dwSerialNo)
                    wprintf(L"\t%s#%d\n", pName1, dwSerialNo);
                else
                    wprintf(L"\t%s\n", pName1);
            }

            wprintf(L"\n");
        }

        if (fIncludeCounters) {
            for (DWORD j = 0; j < g_pObjects[i].dwNumberOfCounters; j++) {
                index = g_pObjects[i].pdwCounterIndexes[j];
                wprintf(L"\t%d %s\n", index, g_pCounterTextHead + g_pTextOffsets[index]);
            }

            wprintf(L"\n\n");
        }
    }

    wprintf(L"\n\n");
}


void FreePerfObjects(PPERF_OBJECT pObjects, DWORD dwNumberOfObjects)
{
    if (pObjects)
    {
        for (DWORD i = 0; i < dwNumberOfObjects; i++)
        {
            if (pObjects[i].pInstanceNames)
            {
                free(pObjects[i].pInstanceNames);
            }

            if (pObjects[i].pdwCounterIndexes)
            {
                free(pObjects[i].pdwCounterIndexes);
            }
        }

        free(pObjects);
        pObjects = NULL;
    }
}