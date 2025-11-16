/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 1998-2015, DataCore Software Corporation. All rights reserved.
 *
 *  Details about the Windows Kernel API are based on the documentation
 *  available at https://learn.microsoft.com/en-us/windows-hardware/drivers/
 */

/*
 *	AVL Map Routines
 */

#include "stdddk.h"


DDKAPI 
VOID RtlInitializeGenericTableAvl(PRTL_AVL_TABLE Table, PRTL_AVL_COMPARE_ROUTINE CompareRoutine,
    PRTL_AVL_ALLOCATE_ROUTINE AllocateRoutine, PRTL_AVL_FREE_ROUTINE FreeRoutine, PVOID TableContext)
{

}

DDKAPI
PVOID RtlInsertElementGenericTableAvl(PRTL_AVL_TABLE Table, PVOID Buffer, CLONG BufferSize, PBOOLEAN NewElement)
{ 
    return nullptr;
}

DDKAPI
PVOID RtlInsertElementGenericTableFullAvl(PRTL_AVL_TABLE Table, PVOID Buffer, CLONG BufferSize,
    PBOOLEAN NewElement, PVOID NodeOrParent, TABLE_SEARCH_RESULT SearchResult)
{
    return nullptr;
}

DDKAPI
BOOLEAN RtlDeleteElementGenericTableAvl(PRTL_AVL_TABLE Table, PVOID Buffer)
{
    return TRUE;
}

DDKAPI
PVOID RtlEnumerateGenericTableAvl(PRTL_AVL_TABLE Table, BOOLEAN Restart)
{
    return nullptr;
}

DDKAPI
PVOID RtlGetElementGenericTableAvl(PRTL_AVL_TABLE Table, ULONG I)
{
    return nullptr;
}

DDKAPI
BOOLEAN RtlIsGenericTableEmptyAvl(PRTL_AVL_TABLE Table)
{
    return TRUE;
}

DDKAPI
PVOID RtlLookupElementGenericTableAvl(PRTL_AVL_TABLE Table, PVOID Buffer)
{
    return nullptr;
}

DDKAPI
PVOID RtlLookupElementGenericTableFullAvl(PRTL_AVL_TABLE Table, PVOID Buffer, 
    PVOID *NodeOrParent, TABLE_SEARCH_RESULT *SearchResult)
{
    return nullptr;
}

DDKAPI
PVOID RtlLookupFirstMatchingElementGenericTableAvl(PRTL_AVL_TABLE Table, PVOID Buffer,
    PVOID *RestartKey)
{
    return nullptr;
}

DDKAPI
ULONG RtlNumberGenericTableElementsAvl(PRTL_AVL_TABLE Table)
{
    return 0;
}
