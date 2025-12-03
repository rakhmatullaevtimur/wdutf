/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 1998-2015, DataCore Software Corporation.
 *  All rights reserved.
 *
 *  Details about the Windows Kernel API are based on the documentation
 *  available at https://learn.microsoft.com/en-us/windows-hardware/drivers/
 */

/*
 *  Rtl AVL Generic Table Tests
 */

#include "stdafx.h"
#include <stdlib.h>

namespace DdkUnitTest
{
    //
    // Element type stored in the AVL table.
    // Key is used as the ordering key for the table.
    //
    typedef struct _AVL_TEST_ELEMENT
    {
        ULONG Key;
        ULONG Value;
    } AVL_TEST_ELEMENT, *PAVL_TEST_ELEMENT;

    //
    // Compare / allocate / free routines for the AVL table.
    // These are used by all tests via InitializeTestAvlTable.
    //
    namespace
    {
        RTL_GENERIC_COMPARE_RESULTS
        NTAPI
        AvlTestCompareRoutine(
            _In_ PRTL_AVL_TABLE Table,
            _In_ PVOID FirstStruct,
            _In_ PVOID SecondStruct
            )
        {
            UNREFERENCED_PARAMETER(Table);

            const AVL_TEST_ELEMENT* left =
                reinterpret_cast<const AVL_TEST_ELEMENT*>(FirstStruct);
            const AVL_TEST_ELEMENT* right =
                reinterpret_cast<const AVL_TEST_ELEMENT*>(SecondStruct);

            if (left->Key < right->Key)
            {
                return GenericLessThan;
            }
            if (left->Key > right->Key)
            {
                return GenericGreaterThan;
            }

            return GenericEqual;
        }

        PVOID
        NTAPI
        AvlTestAllocateRoutine(
            _In_ PRTL_AVL_TABLE Table,
            _In_ CLONG ByteSize
            )
        {
            UNREFERENCED_PARAMETER(Table);

            // For user-mode tests a simple malloc is sufficient.
            return ::malloc(static_cast<size_t>(ByteSize));
        }

        VOID
        NTAPI
        AvlTestFreeRoutine(
            _In_ PRTL_AVL_TABLE Table,
            _In_ PVOID Buffer
            )
        {
            UNREFERENCED_PARAMETER(Table);
            ::free(Buffer);
        }

        VOID
        InitializeTestAvlTable(
            _Out_ PRTL_AVL_TABLE Table,
            _In_ PVOID TableContext = nullptr
            )
        {
            RtlInitializeGenericTableAvl(
                Table,
                AvlTestCompareRoutine,
                AvlTestAllocateRoutine,
                AvlTestFreeRoutine,
                TableContext
            );
        }
    } // anonymous namespace

    TEST_CLASS(DdkRtlAvlTableTest)
    {
    public:

        TEST_METHOD_INITIALIZE(DdkRtlAvlTableTestInit)
        {
            // Required wdutf initialization.
            DdkThreadInit();
        }

        TEST_METHOD(DdkRtlAvlTableContextNull)
        {
            RTL_AVL_TABLE table;
            InitializeTestAvlTable(&table, nullptr);

            Assert::IsTrue(RtlIsGenericTableEmptyAvl(&table));
            Assert::IsTrue(RtlNumberGenericTableElementsAvl(&table) == 0);
            Assert::IsTrue(table.TableContext == nullptr);
        }

        TEST_METHOD(DdkRtlAvlTableContextNotNull)
        {
            RTL_AVL_TABLE table;
            int context;
            InitializeTestAvlTable(&table, &context);

            Assert::IsTrue(RtlIsGenericTableEmptyAvl(&table));
            Assert::IsTrue(RtlNumberGenericTableElementsAvl(&table) == 0);
            Assert::IsTrue(table.TableContext == &context);
        }

        TEST_METHOD(DdkRtlAvlTableLookupDeleteFromEmpty)
        {
            RTL_AVL_TABLE table;
            InitializeTestAvlTable(&table);

            AVL_TEST_ELEMENT key = {};
            key.Key = 42;

            BOOLEAN deleted = RtlDeleteElementGenericTableAvl(&table, &key);
            Assert::IsTrue(deleted == FALSE);

            PAVL_TEST_ELEMENT found =
                reinterpret_cast<PAVL_TEST_ELEMENT>(
                    RtlLookupElementGenericTableAvl(
                        &table,
                        &key
                    )
                );

            Assert::IsTrue(found == nullptr);
        }

        /*
         * Basic check: insert a single element and look it up by key.
         *
         * Covers:
         *  - RtlInitializeGenericTableAvl
         *  - RtlInsertElementGenericTableAvl
         *  - RtlLookupElementGenericTableAvl
         *  - RtlNumberGenericTableElementsAvl
         *  - RtlIsGenericTableEmptyAvl
         */
        TEST_METHOD(DdkRtlAvlTableBasicInsertLookup)
        {
            RTL_AVL_TABLE table;
            InitializeTestAvlTable(&table);

            Assert::IsTrue(RtlIsGenericTableEmptyAvl(&table) != FALSE);
            Assert::IsTrue(RtlNumberGenericTableElementsAvl(&table) == 0);

            AVL_TEST_ELEMENT elem;
            elem.Key = 123;
            elem.Value = 456;

            BOOLEAN isNew = FALSE;

            PAVL_TEST_ELEMENT inserted =
                reinterpret_cast<PAVL_TEST_ELEMENT>(
                    RtlInsertElementGenericTableAvl(
                        &table,
                        &elem,
                        sizeof(elem),
                        &isNew
                    )
                );

            Assert::IsTrue(inserted != nullptr);
            Assert::IsTrue(isNew != FALSE);
            Assert::IsTrue(inserted->Key == elem.Key);
            Assert::IsTrue(inserted->Value == elem.Value);

            Assert::IsTrue(RtlIsGenericTableEmptyAvl(&table) == FALSE);
            Assert::IsTrue(RtlNumberGenericTableElementsAvl(&table) == 1);

            AVL_TEST_ELEMENT lookupKey;
            lookupKey.Key = 123;
            lookupKey.Value = 0;

            PAVL_TEST_ELEMENT found =
                reinterpret_cast<PAVL_TEST_ELEMENT>(
                    RtlLookupElementGenericTableAvl(
                        &table,
                        &lookupKey
                    )
                );

            Assert::IsTrue(found != nullptr);
            Assert::IsTrue(found == inserted);
            Assert::IsTrue(found->Value == 456);

            AVL_TEST_ELEMENT missingKey;
            missingKey.Key = 999;
            missingKey.Value = 0;

            PAVL_TEST_ELEMENT missing =
                reinterpret_cast<PAVL_TEST_ELEMENT>(
                    RtlLookupElementGenericTableAvl(
                        &table,
                        &missingKey
                    )
                );

            Assert::IsTrue(missing == nullptr);
        }

        /*
         * Insert multiple elements and enumerate them in sorted order.
         *
         * Covers:
         *  - RtlInsertElementGenericTableAvl
         *  - RtlEnumerateGenericTableAvl
         *  - RtlNumberGenericTableElementsAvl
         */
        TEST_METHOD(DdkRtlAvlTableInsertAndEnumerate)
        {
            RTL_AVL_TABLE table;
            InitializeTestAvlTable(&table);

            const ULONG keys[]   = { 10, 5, 20, 15, 1 };
            const ULONG values[] = { 100, 50, 200, 150, 10 };

            for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i)
            {
                AVL_TEST_ELEMENT elem;
                elem.Key = keys[i];
                elem.Value = values[i];

                BOOLEAN isNew = FALSE;

                PAVL_TEST_ELEMENT inserted =
                    reinterpret_cast<PAVL_TEST_ELEMENT>(
                        RtlInsertElementGenericTableAvl(
                            &table,
                            &elem,
                            sizeof(elem),
                            &isNew
                        )
                    );

                Assert::IsTrue(inserted != nullptr);
                Assert::IsTrue(isNew != FALSE);
            }

            Assert::IsTrue(
                RtlNumberGenericTableElementsAvl(&table) ==
                (ULONG)(sizeof(keys) / sizeof(keys[0]))
            );

            const ULONG expectedOrder[] = { 1, 5, 10, 15, 20 };

            BOOLEAN restart = TRUE;
            ULONG index = 0;

            for (;;)
            {
                PAVL_TEST_ELEMENT current =
                    reinterpret_cast<PAVL_TEST_ELEMENT>(
                        RtlEnumerateGenericTableAvl(
                            &table,
                            restart
                        )
                    );

                restart = FALSE;

                if (current == nullptr)
                {
                    break;
                }

                Assert::IsTrue(
                    index < (ULONG)(sizeof(expectedOrder) / sizeof(expectedOrder[0]))
                );
                Assert::IsTrue(current->Key == expectedOrder[index]);
                ++index;
            }

            Assert::IsTrue(
                index == (ULONG)(sizeof(expectedOrder) / sizeof(expectedOrder[0]))
            );
        }

        /*
         * Insert the same key twice and verify that the second insert
         * returns the same pointer and does not change element count.
         *
         * Covers:
         *  - RtlInsertElementGenericTableAvl
         *  - RtlNumberGenericTableElementsAvl
         */
        TEST_METHOD(DdkRtlAvlTableDuplicateInsert)
        {
            RTL_AVL_TABLE table;
            InitializeTestAvlTable(&table);

            AVL_TEST_ELEMENT elem;
            elem.Key = 42;
            elem.Value = 1;

            BOOLEAN isNew = FALSE;

            PAVL_TEST_ELEMENT first =
                reinterpret_cast<PAVL_TEST_ELEMENT>(
                    RtlInsertElementGenericTableAvl(
                        &table,
                        &elem,
                        sizeof(elem),
                        &isNew
                    )
                );

            Assert::IsTrue(first != nullptr);
            Assert::IsTrue(isNew != FALSE);

            ULONG countAfterFirst = RtlNumberGenericTableElementsAvl(&table);

            elem.Value = 999;

            BOOLEAN isNew2 = TRUE;

            PAVL_TEST_ELEMENT second =
                reinterpret_cast<PAVL_TEST_ELEMENT>(
                    RtlInsertElementGenericTableAvl(
                        &table,
                        &elem,
                        sizeof(elem),
                        &isNew2
                    )
                );

            Assert::IsTrue(second != nullptr);
            Assert::IsTrue(isNew2 == FALSE);
            Assert::IsTrue(second == first);

            ULONG countAfterSecond = RtlNumberGenericTableElementsAvl(&table);
            Assert::IsTrue(countAfterSecond == countAfterFirst);
        }

        /*
         * Delete an element using the element pointer returned by insert.
         *
         * Covers:
         *  - RtlDeleteElementGenericTableAvl
         *  - RtlNumberGenericTableElementsAvl
         *  - RtlIsGenericTableEmptyAvl
         *  - RtlLookupElementGenericTableAvl
         */
        TEST_METHOD(DdkRtlAvlTableDeleteElement)
        {
            RTL_AVL_TABLE table;
            InitializeTestAvlTable(&table);

            const ULONG keys[] = { 1, 2, 3 };
            PAVL_TEST_ELEMENT elements[3] = {};

            for (size_t i = 0; i < 3; ++i)
            {
                AVL_TEST_ELEMENT elem;
                elem.Key = keys[i];
                elem.Value = (ULONG)(i + 1);

                BOOLEAN isNew = FALSE;

                elements[i] =
                    reinterpret_cast<PAVL_TEST_ELEMENT>(
                        RtlInsertElementGenericTableAvl(
                            &table,
                            &elem,
                            sizeof(elem),
                            &isNew
                        )
                    );

                Assert::IsTrue(elements[i] != nullptr);
                Assert::IsTrue(isNew != FALSE);
            }

            Assert::IsTrue(RtlNumberGenericTableElementsAvl(&table) == 3);
            Assert::IsTrue(RtlIsGenericTableEmptyAvl(&table) == FALSE);

            BOOLEAN deleted =
                RtlDeleteElementGenericTableAvl(
                    &table,
                    elements[1]
                );

            Assert::IsTrue(deleted != FALSE);
            Assert::IsTrue(RtlNumberGenericTableElementsAvl(&table) == 2);

            BOOLEAN deletedAgain =
                RtlDeleteElementGenericTableAvl(
                    &table,
                    elements[1]
                );

            Assert::IsTrue(deletedAgain == FALSE);

            AVL_TEST_ELEMENT lookupKey2;
            lookupKey2.Key = 2;
            lookupKey2.Value = 0;

            PAVL_TEST_ELEMENT found2 =
                reinterpret_cast<PAVL_TEST_ELEMENT>(
                    RtlLookupElementGenericTableAvl(
                        &table,
                        &lookupKey2
                    )
                );

            Assert::IsTrue(found2 == nullptr);

            AVL_TEST_ELEMENT lookupKey1;
            lookupKey1.Key = 1;
            lookupKey1.Value = 0;

            AVL_TEST_ELEMENT lookupKey3;
            lookupKey3.Key = 3;
            lookupKey3.Value = 0;

            PAVL_TEST_ELEMENT found1 =
                reinterpret_cast<PAVL_TEST_ELEMENT>(
                    RtlLookupElementGenericTableAvl(
                        &table,
                        &lookupKey1
                    )
                );

            PAVL_TEST_ELEMENT found3 =
                reinterpret_cast<PAVL_TEST_ELEMENT>(
                    RtlLookupElementGenericTableAvl(
                        &table,
                        &lookupKey3
                    )
                );

            Assert::IsTrue(found1 != nullptr);
            Assert::IsTrue(found3 != nullptr);
        }

        /*
         * Verify empty and non-empty flags and element count.
         *
         * Covers:
         *  - RtlIsGenericTableEmptyAvl
         *  - RtlNumberGenericTableElementsAvl
         */
        TEST_METHOD(DdkRtlAvlTableEmptyFlag)
        {
            RTL_AVL_TABLE table;
            InitializeTestAvlTable(&table);

            Assert::IsTrue(RtlIsGenericTableEmptyAvl(&table) != FALSE);
            Assert::IsTrue(RtlNumberGenericTableElementsAvl(&table) == 0);

            AVL_TEST_ELEMENT elem;
            elem.Key = 1;
            elem.Value = 10;

            BOOLEAN isNew = FALSE;

            PAVL_TEST_ELEMENT inserted =
                reinterpret_cast<PAVL_TEST_ELEMENT>(
                    RtlInsertElementGenericTableAvl(
                        &table,
                        &elem,
                        sizeof(elem),
                        &isNew
                    )
                );

            Assert::IsTrue(inserted != nullptr);
            Assert::IsTrue(isNew != FALSE);

            Assert::IsTrue(RtlIsGenericTableEmptyAvl(&table) == FALSE);
            Assert::IsTrue(RtlNumberGenericTableElementsAvl(&table) == 1);
        }

        /*
         * Insert elements using the "Full" insert API and verify behavior
         * for new and duplicate keys.
         *
         * NodeOrParent and SearchResult are input-only parameters.
         * For stub implementation they may be ignored, but NewElement
         * and the returned pointer must follow the same semantics as
         * RtlInsertElementGenericTableAvl.
         *
         * Covers:
         *  - RtlInsertElementGenericTableFullAvl
         *  - RtlNumberGenericTableElementsAvl
         */
        TEST_METHOD(DdkRtlAvlTableInsertElementFullNewAndDuplicate)
        {
            RTL_AVL_TABLE table;
            InitializeTestAvlTable(&table);

            AVL_TEST_ELEMENT elem1;
            elem1.Key = 5;
            elem1.Value = 100;

            BOOLEAN isNew1 = FALSE;
            PVOID nodeOrParent1 = nullptr;
            TABLE_SEARCH_RESULT searchResult1 = TableEmptyTree;

            PAVL_TEST_ELEMENT first =
                reinterpret_cast<PAVL_TEST_ELEMENT>(
                    RtlInsertElementGenericTableFullAvl(
                        &table,
                        &elem1,
                        sizeof(elem1),
                        &isNew1,
                        nodeOrParent1,
                        searchResult1
                    )
                );

            Assert::IsTrue(first != nullptr);
            Assert::IsTrue(isNew1 != FALSE);
            Assert::IsTrue(RtlNumberGenericTableElementsAvl(&table) == 1);

            AVL_TEST_ELEMENT lookupKey = elem1;

            PVOID nodeOrParentFound = nullptr;
            TABLE_SEARCH_RESULT searchResultFound = TableEmptyTree;

            PAVL_TEST_ELEMENT found =
                reinterpret_cast<PAVL_TEST_ELEMENT>(
                    RtlLookupElementGenericTableFullAvl(
                        &table,
                        &lookupKey,
                        &nodeOrParentFound,
                        &searchResultFound
                    )
                );

            Assert::IsTrue(found != nullptr);
            Assert::IsTrue(found == first);
            Assert::IsTrue(searchResultFound == TableFoundNode);
            Assert::IsTrue(nodeOrParentFound != nullptr);

            AVL_TEST_ELEMENT elem2 = elem1;

            BOOLEAN isNew2 = TRUE;
            PVOID nodeOrParent2 = nodeOrParentFound;
            TABLE_SEARCH_RESULT searchResult2 = searchResultFound;

            PAVL_TEST_ELEMENT second =
                reinterpret_cast<PAVL_TEST_ELEMENT>(
                    RtlInsertElementGenericTableFullAvl(
                        &table,
                        &elem2,
                        sizeof(elem2),
                        &isNew2,
                        nodeOrParent2,
                        searchResult2
                    )
                );

            Assert::IsTrue(second != nullptr);
            Assert::IsTrue(isNew2 == FALSE);
            Assert::IsTrue(second == first);
            Assert::IsTrue(RtlNumberGenericTableElementsAvl(&table) == 1);
        }

        /*
         * Use the "Full" lookup API to search for an existing key
         * and for a missing key.
         *
         * For an existing key:
         *  - SearchResult must be TableFoundNode
         *  - returned pointer must match the one from insert
         *
         * For a missing key:
         *  - returned pointer must be NULL
         *  - SearchResult must not be TableFoundNode
         *
         * NodeOrParent is an out parameter that describes either the
         * found node or the parent where insertion should occur. Tests
         * only require it to be set for the found case.
         *
         * Covers:
         *  - RtlLookupElementGenericTableFullAvl
         */
        TEST_METHOD(DdkRtlAvlTableLookupElementFull)
        {
            RTL_AVL_TABLE table;
            InitializeTestAvlTable(&table);

            AVL_TEST_ELEMENT elem;
            elem.Key = 10;
            elem.Value = 1;

            BOOLEAN isNewInsert = FALSE;

            PAVL_TEST_ELEMENT inserted =
                reinterpret_cast<PAVL_TEST_ELEMENT>(
                    RtlInsertElementGenericTableAvl(
                        &table,
                        &elem,
                        sizeof(elem),
                        &isNewInsert
                    )
                );

            Assert::IsTrue(inserted != nullptr);
            Assert::IsTrue(isNewInsert != FALSE);

            AVL_TEST_ELEMENT lookupKey;
            lookupKey.Key = 10;
            lookupKey.Value = 0;

            PVOID nodeOrParentFound = nullptr;
            TABLE_SEARCH_RESULT searchResultFound = TableEmptyTree;

            PAVL_TEST_ELEMENT found =
                reinterpret_cast<PAVL_TEST_ELEMENT>(
                    RtlLookupElementGenericTableFullAvl(
                        &table,
                        &lookupKey,
                        &nodeOrParentFound,
                        &searchResultFound
                    )
                );

            Assert::IsTrue(found != nullptr);
            Assert::IsTrue(found == inserted);
            Assert::IsTrue(searchResultFound == TableFoundNode);
            Assert::IsTrue(nodeOrParentFound != nullptr);

            AVL_TEST_ELEMENT missingKey;
            missingKey.Key = 999;
            missingKey.Value = 0;

            PVOID nodeOrParentMissing = nullptr;
            TABLE_SEARCH_RESULT searchResultMissing = TableEmptyTree;

            PVOID missing =
                RtlLookupElementGenericTableFullAvl(
                    &table,
                    &missingKey,
                    &nodeOrParentMissing,
                    &searchResultMissing
                );

            Assert::IsTrue(missing == nullptr);
            Assert::IsTrue(searchResultMissing != TableFoundNode);
        }

        /*
         * Use RtlLookupFirstMatchingElementGenericTableAvl to find
         * an element by key and verify that it matches the result
         * of RtlLookupElementGenericTableAvl. RestartKey is an out
         * parameter that should be set for the found case.
         *
         * Covers:
         *  - RtlLookupFirstMatchingElementGenericTableAvl
         */
        TEST_METHOD(DdkRtlAvlTableLookupFirstMatching)
        {
            RTL_AVL_TABLE table;
            InitializeTestAvlTable(&table);

            // lookup in empty map
            AVL_TEST_ELEMENT testKey;
            testKey.Key = 100;
            testKey.Value = 0;

            PVOID restartKeyTest = nullptr;

            PVOID testing =
                RtlLookupFirstMatchingElementGenericTableAvl(
                    &table,
                    &testKey,
                    &restartKeyTest
                );

            Assert::IsTrue(testing == nullptr);
            Assert::IsTrue(restartKeyTest == nullptr);

            const ULONG keys[] = { 5, 15, 25 };

            for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i)
            {
                AVL_TEST_ELEMENT elem;
                elem.Key = keys[i];
                elem.Value = (ULONG)(i + 1);

                BOOLEAN isNew = FALSE;

                PAVL_TEST_ELEMENT inserted =
                    reinterpret_cast<PAVL_TEST_ELEMENT>(
                        RtlInsertElementGenericTableAvl(
                            &table,
                            &elem,
                            sizeof(elem),
                            &isNew
                        )
                    );

                Assert::IsTrue(inserted != nullptr);
                Assert::IsTrue(isNew != FALSE);
            }

            AVL_TEST_ELEMENT lookupKey;
            lookupKey.Key = 15;
            lookupKey.Value = 0;

            PAVL_TEST_ELEMENT foundRegular =
                reinterpret_cast<PAVL_TEST_ELEMENT>(
                    RtlLookupElementGenericTableAvl(
                        &table,
                        &lookupKey
                    )
                );

            Assert::IsTrue(foundRegular != nullptr);

            PVOID restartKey = nullptr;

            PAVL_TEST_ELEMENT foundFirst =
                reinterpret_cast<PAVL_TEST_ELEMENT>(
                    RtlLookupFirstMatchingElementGenericTableAvl(
                        &table,
                        &lookupKey,
                        &restartKey
                    )
                );

            Assert::IsTrue(foundFirst != nullptr);
            Assert::IsTrue(foundFirst == foundRegular);
            Assert::IsTrue(restartKey != nullptr);

            // Lookup of a missing key should return NULL.
            AVL_TEST_ELEMENT missingKey;
            missingKey.Key = 100;
            missingKey.Value = 0;

            PVOID restartKeyMissing = nullptr;

            PVOID missing =
                RtlLookupFirstMatchingElementGenericTableAvl(
                    &table,
                    &missingKey,
                    &restartKeyMissing
                );

            Assert::IsTrue(missing == nullptr);
        }

        /*
         * Walk the AVL tree using RtlRealSuccessor and RtlRealPredecessor
         * and verify that each node's predecessor and successor match
         * the expected in-order neighbors.
         *
         * This test uses keys 0..elementCount-1 and checks that:
         *  - predecessor(key k) == key k-1 for 1 <= k <= N-2
         *  - successor(key k) == key k+1 for 1 <= k <= N-2
         *  - predecessor(minimum) does not point to any element node
         *  - successor(maximum) does not point to any element node
         *
         * Boundary behavior (min/max) is intentionally checked only as
         * "outside the set of element nodes", because the implementation
         * may use an internal sentinel (BalancedRoot) instead of NULL.
         *
         * Covers:
         *  - RtlRealSuccessor
         *  - RtlRealPredecessor
         *  - RtlLookupElementGenericTableFullAvl
         */
        TEST_METHOD(DdkRtlAvlLinksSuccessorPredecessorChain)
        {
            RTL_AVL_TABLE table;
            InitializeTestAvlTable(&table);

            PRTL_BALANCED_LINKS empty_root =
                reinterpret_cast<PRTL_BALANCED_LINKS>(
                    RtlRightChild(
                        reinterpret_cast<PRTL_SPLAY_LINKS>(&table.BalancedRoot))
                );

            Assert::IsTrue(empty_root == nullptr);

            const ULONG elementCount = 16;

            // Insert keys 0..elementCount-1 into the table.
            for (ULONG k = 0; k < elementCount; ++k)
            {
                AVL_TEST_ELEMENT elem;
                elem.Key = k;
                elem.Value = k * 10;

                BOOLEAN isNew = FALSE;
                PAVL_TEST_ELEMENT inserted =
                    reinterpret_cast<PAVL_TEST_ELEMENT>(
                        RtlInsertElementGenericTableAvl(
                            &table,
                            &elem,
                            sizeof(elem),
                            &isNew
                        )
                    );

                Assert::IsTrue(inserted != nullptr);
                Assert::IsTrue(isNew != FALSE);
            }

            // Build an array that maps key -> internal node (PRTL_BALANCED_LINKS).
            PRTL_BALANCED_LINKS nodes[elementCount];

            for (ULONG k = 0; k < elementCount; ++k)
            {
                AVL_TEST_ELEMENT lookupKey;
                lookupKey.Key = k;
                lookupKey.Value = 0;

                PVOID nodeOrParent = nullptr;
                TABLE_SEARCH_RESULT result = TableEmptyTree;

                PAVL_TEST_ELEMENT found =
                    reinterpret_cast<PAVL_TEST_ELEMENT>(
                        RtlLookupElementGenericTableFullAvl(
                            &table,
                            &lookupKey,
                            &nodeOrParent,
                            &result
                        )
                    );

                Assert::IsTrue(found != nullptr);
                Assert::IsTrue(result == TableFoundNode);

                nodes[k] = reinterpret_cast<PRTL_BALANCED_LINKS>(nodeOrParent);
            }

            auto nodeInArray =
                [&](PRTL_BALANCED_LINKS n) -> bool
                {
                    for (ULONG i = 0; i < elementCount; ++i)
                    {
                        if (nodes[i] == n)
                        {
                            return true;
                        }
                    }
                    return false;
                };

            PRTL_BALANCED_LINKS root =
                reinterpret_cast<PRTL_BALANCED_LINKS>(
                    RtlRightChild(
                        reinterpret_cast<PRTL_SPLAY_LINKS>(&table.BalancedRoot))
                );

            Assert::IsTrue(root != nullptr);
            Assert::IsTrue(nodeInArray(root));

            // Check predecessor and successor for inner elements.
            for (ULONG k = 1; k + 1 < elementCount; ++k)
            {
                PRTL_SPLAY_LINKS asSplay =
                    reinterpret_cast<PRTL_SPLAY_LINKS>(nodes[k]);

                PRTL_BALANCED_LINKS pred =
                    reinterpret_cast<PRTL_BALANCED_LINKS>(
                        RtlRealPredecessor(asSplay)
                    );

                PRTL_BALANCED_LINKS succ =
                    reinterpret_cast<PRTL_BALANCED_LINKS>(
                        RtlRealSuccessor(asSplay)
                    );

                Assert::IsTrue(pred == nodes[k - 1]);
                Assert::IsTrue(succ == nodes[k + 1]);
            }

            // Boundary cases: predecessor(min) / successor(max) must not be element nodes.
            {
                PRTL_SPLAY_LINKS minSplay =
                    reinterpret_cast<PRTL_SPLAY_LINKS>(nodes[0]);
                PRTL_SPLAY_LINKS maxSplay =
                    reinterpret_cast<PRTL_SPLAY_LINKS>(nodes[elementCount - 1]);

                PRTL_BALANCED_LINKS predMin =
                    reinterpret_cast<PRTL_BALANCED_LINKS>(
                        RtlRealPredecessor(minSplay)
                    );

                PRTL_BALANCED_LINKS succMax =
                    reinterpret_cast<PRTL_BALANCED_LINKS>(
                        RtlRealSuccessor(maxSplay)
                    );

                // Either NULL or sentinel, but must not be any element node.
                Assert::IsFalse(nodeInArray(predMin));
                Assert::IsFalse(nodeInArray(succMax));
            }
        }

        /*
         * Use RtlLeftChild and RtlRightChild to walk from the root
         * to the minimum and maximum elements and verify that they
         * match the expected nodes.
         *
         * The tree is populated with keys 0..elementCount-1.
         * We expect:
         *  - leftmost node reachable from the root to be key 0
         *  - rightmost node reachable from the root to be key elementCount-1
         *
         * Covers:
         *  - RtlLeftChild
         *  - RtlRightChild
         *  - RtlLookupElementGenericTableFullAvl
         */
        TEST_METHOD(DdkRtlAvlLinksLeftRightChildrenReachExtremes)
        {
            RTL_AVL_TABLE table;
            InitializeTestAvlTable(&table);

            const ULONG elementCount = 16;

            // Insert keys 0..elementCount-1 into the table.
            for (ULONG k = 0; k < elementCount; ++k)
            {
                AVL_TEST_ELEMENT elem;
                elem.Key = k;
                elem.Value = k * 10;

                BOOLEAN isNew = FALSE;
                PAVL_TEST_ELEMENT inserted =
                    reinterpret_cast<PAVL_TEST_ELEMENT>(
                        RtlInsertElementGenericTableAvl(
                            &table,
                            &elem,
                            sizeof(elem),
                            &isNew
                        )
                    );

                Assert::IsTrue(inserted != nullptr);
                Assert::IsTrue(isNew != FALSE);
            }

            // Build an array that maps key -> internal node (PRTL_BALANCED_LINKS).
            PRTL_BALANCED_LINKS nodes[elementCount];

            for (ULONG k = 0; k < elementCount; ++k)
            {
                AVL_TEST_ELEMENT lookupKey;
                lookupKey.Key = k;
                lookupKey.Value = 0;

                PVOID nodeOrParent = nullptr;
                TABLE_SEARCH_RESULT result = TableEmptyTree;

                PAVL_TEST_ELEMENT found =
                    reinterpret_cast<PAVL_TEST_ELEMENT>(
                        RtlLookupElementGenericTableFullAvl(
                            &table,
                            &lookupKey,
                            &nodeOrParent,
                            &result
                        )
                    );

                Assert::IsTrue(found != nullptr);
                Assert::IsTrue(result == TableFoundNode);

                nodes[k] = reinterpret_cast<PRTL_BALANCED_LINKS>(nodeOrParent);
            }

            // In an AVL table the logical root node is the right child of BalancedRoot.
            PRTL_BALANCED_LINKS root =
                reinterpret_cast<PRTL_BALANCED_LINKS>(
                    RtlRightChild(
                        reinterpret_cast<PRTL_SPLAY_LINKS>(&table.BalancedRoot))
                );

            Assert::IsTrue(root != nullptr);

            // Walk down the left children until we reach the leftmost node.
            PRTL_BALANCED_LINKS leftMost = root;
            while (RtlLeftChild(reinterpret_cast<PRTL_SPLAY_LINKS>(leftMost)) != nullptr)
            {
                leftMost =
                    reinterpret_cast<PRTL_BALANCED_LINKS>(
                        RtlLeftChild(reinterpret_cast<PRTL_SPLAY_LINKS>(leftMost))
                    );
            }

            // The leftmost node must correspond to key 0.
            Assert::IsTrue(leftMost == nodes[0]);

            // Walk down the right children until we reach the rightmost node.
            PRTL_BALANCED_LINKS rightMost = root;
            while (RtlRightChild(reinterpret_cast<PRTL_SPLAY_LINKS>(rightMost)) != nullptr)
            {
                rightMost =
                    reinterpret_cast<PRTL_BALANCED_LINKS>(
                        RtlRightChild(reinterpret_cast<PRTL_SPLAY_LINKS>(rightMost))
                    );
            }

            // The rightmost node must correspond to key elementCount-1.
            Assert::IsTrue(rightMost == nodes[elementCount - 1]);
        }

        /*
         * Perform a full in-order traversal using RtlRealSuccessor,
         * starting from the leftmost node reached via RtlLeftChild.
         *
         * The tree is populated with keys 0..elementCount-1.
         * We expect:
         *  - the first node to be key 0
         *  - each call to RtlRealSuccessor to move to the node for key+1
         *  - traversal to end after visiting all elements
         *
         * Covers:
         *  - RtlLeftChild
         *  - RtlRightChild
         *  - RtlRealSuccessor
         *  - RtlLookupElementGenericTableFullAvl
         */
        TEST_METHOD(DdkRtlAvlLinksInorderTraversalUsingSuccessor)
        {
            RTL_AVL_TABLE table;
            InitializeTestAvlTable(&table);

            const ULONG elementCount = 16;

            // Insert keys 0..elementCount-1 into the table.
            for (ULONG k = 0; k < elementCount; ++k)
            {
                AVL_TEST_ELEMENT elem;
                elem.Key = k;
                elem.Value = k * 10;

                BOOLEAN isNew = FALSE;
                PAVL_TEST_ELEMENT inserted =
                    reinterpret_cast<PAVL_TEST_ELEMENT>(
                        RtlInsertElementGenericTableAvl(
                            &table,
                            &elem,
                            sizeof(elem),
                            &isNew
                        )
                    );

                Assert::IsTrue(inserted != nullptr);
                Assert::IsTrue(isNew != FALSE);
            }

            // Build an array that maps key -> internal node (PRTL_BALANCED_LINKS).
            PRTL_BALANCED_LINKS nodes[elementCount];

            for (ULONG k = 0; k < elementCount; ++k)
            {
                AVL_TEST_ELEMENT lookupKey;
                lookupKey.Key = k;
                lookupKey.Value = 0;

                PVOID nodeOrParent = nullptr;
                TABLE_SEARCH_RESULT result = TableEmptyTree;

                PAVL_TEST_ELEMENT found =
                    reinterpret_cast<PAVL_TEST_ELEMENT>(
                        RtlLookupElementGenericTableFullAvl(
                            &table,
                            &lookupKey,
                            &nodeOrParent,
                            &result
                        )
                    );

                Assert::IsTrue(found != nullptr);
                Assert::IsTrue(result == TableFoundNode);

                nodes[k] = reinterpret_cast<PRTL_BALANCED_LINKS>(nodeOrParent);
            }

            // Get the logical root of the tree.
            PRTL_BALANCED_LINKS root =
                reinterpret_cast<PRTL_BALANCED_LINKS>(
                    RtlRightChild(
                        reinterpret_cast<PRTL_SPLAY_LINKS>(&table.BalancedRoot))
                );
            Assert::IsTrue(root != nullptr);

            // Find the leftmost node by following left children from the root.
            PRTL_BALANCED_LINKS current = root;
            while (RtlLeftChild(reinterpret_cast<PRTL_SPLAY_LINKS>(current)) != nullptr)
            {
                current =
                    reinterpret_cast<PRTL_BALANCED_LINKS>(
                        RtlLeftChild(reinterpret_cast<PRTL_SPLAY_LINKS>(current))
                    );
            }

            // The leftmost node must correspond to key 0.
            Assert::IsTrue(current == nodes[0]);

            // Walk the tree in-order using RtlRealSuccessor and verify that
            // we visit nodes in the order of keys 0..elementCount-1.
            ULONG index = 0;
            while (current != nullptr)
            {
                Assert::IsTrue(index < elementCount);
                Assert::IsTrue(current == nodes[index]);

                current =
                    reinterpret_cast<PRTL_BALANCED_LINKS>(
                        RtlRealSuccessor(
                            reinterpret_cast<PRTL_SPLAY_LINKS>(current))
                    );
                ++index;
            }

            // All elements must have been visited exactly once.
            Assert::IsTrue(index == elementCount);
        }

        TEST_METHOD(DdkRtlAvlLinksClearThroughRoot)
        {
            RTL_AVL_TABLE table;
            InitializeTestAvlTable(&table);

            const ULONG elementCount = 16;

            // Insert keys 0..elementCount-1 into the table.
            for (ULONG k = 0; k < elementCount; ++k)
            {
                AVL_TEST_ELEMENT elem;
                elem.Key = k;
                elem.Value = k * 10;

                BOOLEAN isNew = FALSE;
                PAVL_TEST_ELEMENT inserted =
                    reinterpret_cast<PAVL_TEST_ELEMENT>(
                        RtlInsertElementGenericTableAvl(
                            &table,
                            &elem,
                            sizeof(elem),
                            &isNew
                        )
                    );

                Assert::IsTrue(inserted != nullptr);
                Assert::IsTrue(isNew != FALSE);
            }

            ULONG sz = RtlNumberGenericTableElementsAvl(&table);
            Assert::IsTrue(sz == elementCount);

            for (ULONG k = 0; k < elementCount; ++k)
            {
                // Get the logical root of the tree.
                PRTL_BALANCED_LINKS root =
                    reinterpret_cast<PRTL_BALANCED_LINKS>(
                        RtlRightChild(
                            reinterpret_cast<PRTL_SPLAY_LINKS>(&table.BalancedRoot))
                    );
                Assert::IsTrue(root != nullptr);

                BOOLEAN res = RtlDeleteElementGenericTableAvl(&table, (root+1));
                Assert::IsTrue(res);
            }
            
            sz = RtlNumberGenericTableElementsAvl(&table);
            Assert::IsTrue(sz == 0);

            // Get the logical root of the tree.
            PRTL_BALANCED_LINKS empty_root =
                reinterpret_cast<PRTL_BALANCED_LINKS>(
                    RtlRightChild(
                        reinterpret_cast<PRTL_SPLAY_LINKS>(&table.BalancedRoot))
                );
            Assert::IsTrue(empty_root == nullptr);
        }
    };
}
