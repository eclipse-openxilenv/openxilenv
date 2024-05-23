/*
 * Copyright 2023 ZF Friedrichshafen AG
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <stdint.h>

#include "MyMemory.h"
#include "ThrowError.h"

#include "BlackboardHashIndex.h"

typedef struct {
    uint64_t Hash[512];
    int32_t Indexes[512];
    int32_t Pos;
} HASH_INDEXES;

typedef struct {
    uint64_t Hash;
    HASH_INDEXES *Indexes;
} HASH_INDEXES_ELEM;

typedef struct {
    HASH_INDEXES_ELEM *Elems;
    int32_t Pos;
    uint32_t Size;
} MASTER_HASH_INDEXES;

static MASTER_HASH_INDEXES MasterHashIndex;
static int *FreeIndexes;
static int FreeIndexesSize;
static int FreeIndexesPos;

uint64_t BuildHashCode(const char* Name)
{
    uint64_t Ret = 0;
    while (*Name != 0) {
        // Eigentliche Berechnung
        Ret = Ret * 31 + (uint64_t)*Name++;
        // Nur fuer Debug-Zwecke damit es auch mal zu gleichen Hash-Werten kommen kann
        // Ret = Ret + *Name++;
    }
    return Ret;
}

#if 0
static int32_t BinarySearchHighestMost1(uint64_t HashCode)
{
    int32_t l = 0;
    int32_t r = (int32_t)MasterHashIndex.Pos;
    while (l < r) {
        int32_t m = (l + r) >> 1;
        if (MasterHashIndex.Elems[m].Hash < HashCode) {
            l = m + 1;
        } else {
            r = m;
        }
    }
    return l;    // return Size if Value larger than largestes inside Array
}

static int32_t BinarySearchHighestMost2(HASH_INDEXES *Elems, uint64_t HashCode)
{
    int32_t l = 0;
    int32_t r = (int32_t)Elems->Pos;
    while (l < r) {
        int32_t m = (l + r) >> 1;
        if (Elems->Hash[m] > HashCode) {
            l = m + 1;
        } else {
            r = m;
        }
    }
    return l;   // return Size if Value larger than largestes inside Array
}
#endif

static int32_t BinarySearchLowestMost1(uint64_t HashCode)
{
    int32_t l = 0;
    int32_t r = (int32_t)MasterHashIndex.Pos;
    while (l < r) {
        int32_t m = (l + r) >> 1;
        if (MasterHashIndex.Elems[m].Hash > HashCode) {
            r = m;
        } else {
            l = m + 1;
        }
    }
    return l - 1;  // return -1 if Value smaller than smallest inside Array
}

static int32_t BinarySearchLowestMost2(HASH_INDEXES *Elems, uint64_t HashCode)
{
    int32_t l = 0;
    int32_t r = (int32_t)Elems->Pos;
    while (l < r) {
        int32_t m = (l + r) >> 1;
        if (Elems->Hash[m] > HashCode) {
            r = m;
        } else {
            l = m + 1;
        }
    }
    return l - 1;
}


int BinaryHashSerchIndex (uint64_t Hash, int32_t *ret_p1, int32_t *ret_p2)
{
    HASH_INDEXES *HashIndex;
    int32_t p1, p2;

    if (MasterHashIndex.Pos < 1) {
        *ret_p1 = 0;
        *ret_p2 = 0;
        return -1;
    }

    p1 = BinarySearchLowestMost1(Hash);
    if (p1 < 0) p1 = 0;
    HashIndex = MasterHashIndex.Elems[p1].Indexes;
    p2 = BinarySearchLowestMost2(HashIndex, Hash);
    if (p2 < 0) p2 = 0;
    *ret_p1 = p1;
    *ret_p2 = p2;
    if (Hash == HashIndex->Hash[p2]) {
        return (int)HashIndex->Indexes[p2];
    } else {
        return -1;
    }
}

int GetNextIndexBeforeSameHash (uint64_t Hash, int32_t *ret_p1, int32_t *ret_p2)
{
    HASH_INDEXES *HashIndex;
    if (*ret_p2 > 0) {
        // ist noch im gleichen Block
        *ret_p2 = *ret_p2 - 1;
        HashIndex = MasterHashIndex.Elems[*ret_p1].Indexes;
    } else {
        // ist nicht mehr im gleichen Block
        *ret_p1 = *ret_p1 - 1;
        HashIndex = MasterHashIndex.Elems[*ret_p1].Indexes;
        *ret_p2 = HashIndex->Pos - 1;
    }
    if (HashIndex->Hash[*ret_p2] == Hash) {
        return HashIndex->Indexes[*ret_p2];   // es gibt einen Index davor mit gleichem HashCode
    } else {
        return -1;
    }
}

int GetNextIndexBehindSameHash (uint64_t Hash, int32_t *ret_p1, int32_t *ret_p2)
{
    HASH_INDEXES *HashIndex;
    HashIndex = MasterHashIndex.Elems[*ret_p1].Indexes;
    if (*ret_p2 < (HashIndex->Pos - 1)) {
        // ist noch im gleichen Block
        *ret_p2 = *ret_p2 + 1;
    } else {
        // ist nicht mehr im gleichen Block
        *ret_p1 = *ret_p1 + 1;
        if (*ret_p1 >= MasterHashIndex.Pos) return -1;     // letzes Element
        HashIndex = MasterHashIndex.Elems[*ret_p1].Indexes;
        *ret_p2 = 0;
    }
    if (HashIndex->Hash[*ret_p2] == Hash) {
        return HashIndex->Indexes[*ret_p2];   // es gibt einen Index davor mit gleichem HashCode
    } else {
        return -1;
    }
}

#define UNUSED(x) (void)(x)
static void InsertHashIntoBlock(HASH_INDEXES* HashIndex, uint64_t Hash, int32_t Index, int32_t p1, int32_t p2)
{
    UNUSED(p1);
    int32_t x;
    if ((p2 == 0) && (Hash < HashIndex->Hash[0])) {
        for (x = HashIndex->Pos; x > 0; x--) {
            HashIndex->Hash[x] = HashIndex->Hash[x-1];
            HashIndex->Indexes[x] = HashIndex->Indexes[x-1];
        }
        HashIndex->Hash[0] = Hash;
        HashIndex->Indexes[0] = Index;
    } else {
        for (x = HashIndex->Pos; x > (p2+1); x--) {
            HashIndex->Hash[x] = HashIndex->Hash[x-1];
            HashIndex->Indexes[x] = HashIndex->Indexes[x-1];
        }
        HashIndex->Hash[p2+1] = Hash;
        HashIndex->Indexes[p2+1] = Index;
    }
    HashIndex->Pos++;
}

int AddBinaryHashKey(uint64_t Hash, int32_t Index, int32_t p1, int32_t p2)
{
    HASH_INDEXES *HashIndex;
    int32_t x, y;

    if (MasterHashIndex.Pos == 0) {
        // Es gibt noch gar keinen Eintrag!
        HASH_INDEXES* NewHashBlock = my_malloc (sizeof(HASH_INDEXES));
        if (NewHashBlock == NULL) return -1;
        NewHashBlock->Hash[0] = Hash;
        NewHashBlock->Indexes[0] = Index;
        NewHashBlock->Pos = 1;
        MasterHashIndex.Elems[0].Hash = Hash;
        MasterHashIndex.Elems[0].Indexes = NewHashBlock;
        MasterHashIndex.Pos = 1;
    } else {
        HashIndex = MasterHashIndex.Elems[p1].Indexes;
        if (HashIndex->Pos < 512) {
            // passt noch rein
            InsertHashIntoBlock(HashIndex, Hash, Index, p1, p2);
        } else {
            // Passt nicht mehr rein -> neuer Hash-Block und teilen
            HASH_INDEXES* NewHashBlock = my_malloc (sizeof(HASH_INDEXES));
            if (NewHashBlock == NULL) return -1;
            for (y = 0, x = HashIndex->Pos / 2; x < HashIndex->Pos; x++, y++) {
                NewHashBlock->Hash[y] = HashIndex->Hash[x];
                NewHashBlock->Indexes[y] = HashIndex->Indexes[x];
            }
            NewHashBlock->Pos = HashIndex->Pos = HashIndex->Pos / 2;
            /*if (NewHashBlock->Pos > 512) {
                printf ("Stop\n");
            }*/

            // neuer Block hinter dem alten einsortieren
            for (x = MasterHashIndex.Pos; x > (p1+1) ; x--) {
                MasterHashIndex.Elems[x].Hash = MasterHashIndex.Elems[x-1].Hash;
                MasterHashIndex.Elems[x].Indexes = MasterHashIndex.Elems[x-1].Indexes;
            }
            MasterHashIndex.Elems[p1+1].Hash = NewHashBlock->Hash[0];
            MasterHashIndex.Elems[p1+1].Indexes = NewHashBlock;
            MasterHashIndex.Pos++;

            if (p2 >= HashIndex->Pos) {
                // in neuen Block einsortieren
                p2 -= HashIndex->Pos;
                InsertHashIntoBlock(NewHashBlock, Hash, Index, p1, p2);
            } else {
                // in alten Block einsortieren
                InsertHashIntoBlock(HashIndex, Hash, Index, p1, p2);
            }
        }
    }
    return 0;
}

void RemoveBinaryHashKey(int32_t p1, int32_t p2)
{
    HASH_INDEXES *HashIndex;
    int32_t x, y;

    HashIndex = MasterHashIndex.Elems[p1].Indexes;
    for (x = p2; x < (HashIndex->Pos-1); x++) {
        HashIndex->Hash[x] = HashIndex->Hash[x+1];
        HashIndex->Indexes[x] = HashIndex->Indexes[x+1];
    }
    HashIndex->Pos--;
    /*if (HashIndex->Pos = 512) {
        printf ("Stop\n");
    }*/

    // wenn weniger wie 16 Eintraege versuche die Hash-Tabelle zu mergen.
    if (HashIndex->Pos < 16) {
        if (p1 > 0) {
            HASH_INDEXES *HashIndexBefore = MasterHashIndex.Elems[p1-1].Indexes;
            if (HashIndexBefore->Pos < 256) {
                for (y = HashIndexBefore->Pos, x = 0; x < HashIndex->Pos; y++, x++) {
                    HashIndexBefore->Hash[y] = HashIndex->Hash[x];
                    HashIndexBefore->Indexes[y] = HashIndex->Indexes[x];
                }
                HashIndexBefore->Pos = y;

                my_free(HashIndex);
                for(x = p1; x < (MasterHashIndex.Pos - 1); x++) {
                    MasterHashIndex.Elems[x].Hash = MasterHashIndex.Elems[x + 1].Hash;
                    MasterHashIndex.Elems[x].Indexes = MasterHashIndex.Elems[x + 1].Indexes;
                }
                MasterHashIndex.Pos--;
            }
        }
    } else if (HashIndex->Pos < 256) {
        if (p1 < (MasterHashIndex.Pos - 1)) {
            HASH_INDEXES *HashIndexBehind = MasterHashIndex.Elems[p1+1].Indexes;
            if (HashIndexBehind->Pos < 16) {
                for (y = HashIndex->Pos, x = 0; x < HashIndexBehind->Pos; y++, x++) {
                    HashIndex->Hash[y] = HashIndexBehind->Hash[x];
                    HashIndex->Indexes[y] = HashIndexBehind->Indexes[x];
                }
                HashIndex->Pos = y;
                /*if (HashIndex->Pos > 512) {
                    printf ("Stop\n");
                }*/
                my_free(HashIndexBehind);
                for(x = p1+1; x < (MasterHashIndex.Pos - 1); x++) {
                    MasterHashIndex.Elems[x].Hash = MasterHashIndex.Elems[x + 1].Hash;
                    MasterHashIndex.Elems[x].Indexes = MasterHashIndex.Elems[x + 1].Indexes;
                }
                MasterHashIndex.Pos--;
            }
        }
    }
}

int GetFreeBlackboardIndex(void)
{
    if (FreeIndexesPos == 0) return -1;    // Blackboard ist voll
    FreeIndexesPos--;
    return FreeIndexes[FreeIndexesPos];
}

void FreeBlackboardIndex(int Index)
{
    FreeIndexesPos++;
    if (FreeIndexesPos > FreeIndexesSize) {
        ThrowError (1, "internal error more free blackboard indexes as expected");
    } else {
        FreeIndexes[FreeIndexesPos-1] = Index;
    }
}


int InitMasterHashTable(int BlackboardSize)
{
    int x;

    MasterHashIndex.Size = 512;
    MasterHashIndex.Pos = 0;
    MasterHashIndex.Elems = (HASH_INDEXES_ELEM*)my_malloc(512 * sizeof(HASH_INDEXES_ELEM));
    if (MasterHashIndex.Elems == NULL) return -1;

    FreeIndexesSize = BlackboardSize;
    FreeIndexesPos = BlackboardSize;
    FreeIndexes = (int*)my_malloc((size_t)BlackboardSize * sizeof(int));
    if (FreeIndexes == NULL) return -1;
    for (x = 0; x < BlackboardSize; x++) {
        FreeIndexes[x] = x;
    }

    return 0;
}

int CloseMasterHashTable(void)
{
    int32_t x;
    HASH_INDEXES_ELEM *e = MasterHashIndex.Elems;
    for (x = 0; x < MasterHashIndex.Pos; x++) {
        if (e->Indexes != NULL) my_free(e->Indexes);
    }
    my_free(MasterHashIndex.Elems);
    MasterHashIndex.Elems = NULL;
    my_free(FreeIndexes);
    FreeIndexes = NULL;
    return 0;
}
