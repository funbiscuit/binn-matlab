#include <string.h>
#include <stdint.h>

#include "mex.h"

#include "binn.h"

#if !defined(max)
#define	max(A, B)	((A) > (B) ? (A) : (B))
#endif

#if !defined(min)
#define	min(A, B)	((A) < (B) ? (A) : (B))
#endif

// retrieves binn number value and assignes it to given pointer to number
#define GET_BINN_NUM(value, pnum) \
    if((value).type == BINN_UINT8) {          \
    *(pnum) = (value).vuint8;                 \
    } else if((value).type == BINN_INT8) {    \
    *(pnum) = (value).vint8;                  \
    } else if((value).type == BINN_UINT16) {  \
    *(pnum) = (value).vuint16;                \
    } else if((value).type == BINN_INT16) {   \
    *(pnum) = (value).vint16;                 \
    } else if((value).type == BINN_UINT32) {  \
    *(pnum) = (value).vuint32;                \
    } else if((value).type == BINN_INT32) {   \
    *(pnum) = (value).vint32;                 \
    } else if((value).type == BINN_UINT64) {  \
    *(pnum) = (value).vuint64;                \
    } else if((value).type == BINN_INT64) {   \
    *(pnum) = (value).vint64;                 \
    } else if((value).type == BINN_FLOAT) {   \
    *(pnum) = (value).vfloat;                 \
    } else if((value).type == BINN_DOUBLE) {  \
    *(pnum) = (value).vdouble;                \
    }                                         \

#define TYPE_MAP_LEN (sizeof(typeMap)/sizeof(*typeMap))

static int typeMap[][2] = {
        {BINN_UINT8,  mxUINT8_CLASS},
        {BINN_INT8,   mxINT8_CLASS},
        {BINN_UINT16, mxUINT16_CLASS},
        {BINN_INT16,  mxINT16_CLASS},
        {BINN_UINT32, mxUINT32_CLASS},
        {BINN_INT32,  mxINT32_CLASS},
        {BINN_UINT64, mxUINT64_CLASS},
        {BINN_INT64,  mxINT64_CLASS},
        {BINN_SINGLE, mxSINGLE_CLASS},
        {BINN_DOUBLE, mxDOUBLE_CLASS},
};

mxArray *decode(binn *obj);

mxArray *decodeNumber(binn *obj, int mxClassId, int numberSize) {
    mwSize dims = 1;

    mxArray *val = mxCreateNumericArray(1, &dims, mxClassId, mxREAL);
    memcpy(mxGetPr(val), &obj->vint, numberSize);

    return val;
}

mxArray *decodeObject(binn *obj) {
    binn_iter iter;
    binn value;
    char key[256];

    binn_iter_init(&iter, obj, BINN_OBJECT);
    int count = iter.count;
    char **fieldNames = malloc(count * sizeof(char *));

    // get all field names since we need them before struct is created
    int i = 0;
    binn_object_foreach(obj, key, value) {
        fieldNames[i] = malloc(strlen(key) + 1);
        memcpy(fieldNames[i], key, strlen(key));
        fieldNames[i][strlen(key)] = '\0';
        ++i;
    }
    mwSize dims = 1;
    mxArray *decodedObj = mxCreateStructArray(1, &dims, count, (const char **) fieldNames);

    i = 0;
    binn_object_foreach(obj, key, value) {
        mxArray *field_value = decode(&value);
        if (field_value != NULL) {
            mxSetFieldByNumber(decodedObj, 0, i, field_value);
        } else {
            mexPrintf("Skipped field '%s' with unknown or invalid value\n", key);
        }
        ++i;
    }

    // free allocated space for field names
    for (int j = 0; j < count; ++j) {
        free(fieldNames[j]);
    }
    free(fieldNames);
    return decodedObj;
}

bool isSupportedNumber(int type) {
    int storage = type & BINN_STORAGE_MASK;
    type &= BINN_TYPE_MASK;

    if (type > 2) {
        return false;
    }
    if (storage != BINN_STORAGE_BYTE && storage != BINN_STORAGE_WORD &&
        storage != BINN_STORAGE_DWORD && storage != BINN_STORAGE_QWORD) {
        return false;
    }
    if (type == 2 && storage < BINN_STORAGE_DWORD) {
        return false;
    }

    return true;
}

mxArray *decodeList(binn *obj) {
    binn_iter iter;
    binn value;

    binn_iter_init(&iter, obj, BINN_LIST);
    mwSize itemCount = iter.count;

    if (itemCount == 0) {
        return mxCreateDoubleMatrix(0, 0, mxREAL);
    }

    // we need to determine what type to use for created array since in
    // binn list values of different types can be stored
    // in future we can convert lists of hybrid data into cell arrays

    // the following approach is far from optimal

    binn_list_get_value(obj, 1, &value);
    int bestType = binn_type(&value);

    int bestStorage = bestType & BINN_STORAGE_MASK;
    bestType &= BINN_TYPE_MASK;

    bool hasInts = false;
    uint64_t maxUint = 0;

    binn_list_foreach(obj, value) {
        int type = binn_type(&value);

        if(!isSupportedNumber(type)) {
            mexWarnMsgIdAndTxt("MATLAB:binnDecode:unknownType", "List contains unsupported type: 0x%02X", type);
            return NULL;
        }

        int storage = type & BINN_STORAGE_MASK;
        type &= BINN_TYPE_MASK;

        if (type == 2) {
            bestType = 2;
            // List contains float/double values so converting to ints is
            // not possible. If we don't have integers and doubles we can get
            // away with single precision
            if (storage == BINN_STORAGE_DWORD && bestStorage != BINN_STORAGE_QWORD && !hasInts) {
                bestStorage = BINN_STORAGE_DWORD;
            } else {
                bestStorage = BINN_STORAGE_QWORD;
            }
        } else {
            hasInts = true;

            if (bestType == 2) {
                bestStorage = BINN_STORAGE_QWORD;
                continue;
            }
            bestStorage = max(storage, bestStorage);

            // in case we have integers of different sign, we can only
            // convert them to signed integers
            if (type != bestType) {
                bestType = 1;
            }

            // remember maximal unsigned value since we might need to
            // squish it into signed type
            if (type == 0) {
                if (storage == BINN_STORAGE_BYTE) {
                    maxUint = max(maxUint, value.vuint8);
                } else if (storage == BINN_STORAGE_WORD) {
                    maxUint = max(maxUint, value.vuint16);
                } else if (storage == BINN_STORAGE_DWORD) {
                    maxUint = max(maxUint, value.vuint32);
                } else {
                    maxUint = max(maxUint, value.vuint64);
                }
            }
        }
    }

    if (bestType == 1) {
        // check if we can fit max uint into best int type
        uint64_t maxInt;
        int nextStorage = 0;
        if (bestStorage == BINN_STORAGE_BYTE) {
            maxInt = INT8_MAX;
            nextStorage = BINN_STORAGE_WORD;
        } else if (bestStorage == BINN_STORAGE_WORD) {
            maxInt = INT16_MAX;
            nextStorage = BINN_STORAGE_DWORD;
        } else if (bestStorage == BINN_STORAGE_DWORD) {
            maxInt = INT32_MAX;
            nextStorage = BINN_STORAGE_QWORD;
        } else {
            maxInt = INT64_MAX;
        }

        if (maxInt < maxUint) {
            // can't fit maximum stored uint into int, increase storage
            if (nextStorage != 0) {
                bestStorage = nextStorage;
            } else {
                // very big integers, have to convert them to doubles and lose
                // precision, but that's the only option
                bestType = 2;
                bestStorage = BINN_STORAGE_QWORD;
            }
        }
    }
    mxClassID classId = mxUNKNOWN_CLASS;
    for (int i = 0; i < TYPE_MAP_LEN; ++i) {
        if (typeMap[i][0] == (bestStorage | bestType)) {
            classId = typeMap[i][1];
            break;
        }
    }
    if (classId == mxUNKNOWN_CLASS) {
        // should not happen
        return NULL;
    }

    mxArray *val = mxCreateNumericArray(1, &itemCount, classId, mxREAL);
    void *arr = mxGetPr(val);

    int i = 0;
    binn_list_foreach(obj, value) {
        if (classId == mxUINT8_CLASS) {
            uint8_t *num = &((uint8_t *) arr)[i];
            GET_BINN_NUM(value, num)
        } else if (classId == mxINT8_CLASS) {
            int8_t *num = &((int8_t *) arr)[i];
            GET_BINN_NUM(value, num)
        } else if (classId == mxUINT16_CLASS) {
            uint16_t *num = &((uint16_t *) arr)[i];
            GET_BINN_NUM(value, num)
        } else if (classId == mxINT16_CLASS) {
            int16_t *num = &((int16_t *) arr)[i];
            GET_BINN_NUM(value, num)
        } else if (classId == mxUINT32_CLASS) {
            uint32_t *num = &((uint32_t *) arr)[i];
            GET_BINN_NUM(value, num)
        } else if (classId == mxINT32_CLASS) {
            int32_t *num = &((int32_t *) arr)[i];
            GET_BINN_NUM(value, num)
        } else if (classId == mxUINT64_CLASS) {
            uint64_t *num = &((uint64_t *) arr)[i];
            GET_BINN_NUM(value, num)
        } else if (classId == mxINT64_CLASS) {
            int64_t *num = &((int64_t *) arr)[i];
            GET_BINN_NUM(value, num)
        } else if (classId == mxSINGLE_CLASS) {
            float *num = &((float *) arr)[i];
            GET_BINN_NUM(value, num)
        } else if (classId == mxDOUBLE_CLASS) {
            double *num = &((double *) arr)[i];
            GET_BINN_NUM(value, num)
        }

        ++i;
    }

    return val;
}

mxArray *decode(binn *obj) {
    //TODO error checking

    if (obj->type == BINN_OBJECT) {
        return decodeObject(obj);
    } else if (obj->type == BINN_LIST) {
        return decodeList(obj);
    } else if (obj->type == BINN_UINT8) {
        return decodeNumber(obj, mxUINT8_CLASS, sizeof(uint8_t));
    } else if (obj->type == BINN_DOUBLE) {
        return decodeNumber(obj, mxDOUBLE_CLASS, sizeof(double));
    } else if (obj->type == BINN_STRING) {
        return mxCreateCharMatrixFromStrings(1, (const char **) &(obj->ptr));
    }

    mexWarnMsgIdAndTxt("MATLAB:binnDecode:unknownType", "Unknown binn type: 0x%02X\n", obj->type);
    return NULL;
}


void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[]) {

    // check number of arguments
    if (nrhs != 1) {
        mexErrMsgTxt("One input is required.");
    } else if (nlhs > 1) {
        mexErrMsgTxt("Too many output arguments.");
    }

    const mxArray *initial = prhs[0];


    if (!mxIsNumeric(initial)) {
        mexErrMsgTxt("First input argument is not numeric ");
    }
    if (mxIsEmpty(initial) || !mxIsUint8(initial) || mxIsComplex(initial)) {
        mexErrMsgTxt("First input argument is not a non empty uint8 array");
    }
    if (mxGetM(initial) != 1 && mxGetN(initial) != 1) {
        mexErrMsgTxt("First input argument is not a vector");
    }

    binn *root = NULL;

    root = binn_open(mxGetPr(initial));
    if (root == NULL) {
        mexErrMsgTxt("Input argument is not a valid binn");
    } else if (root->type != BINN_OBJECT) {
        mexErrMsgTxt("Input argument is not a binn object");
    }

    plhs[0] = decode(root);

    binn_free(root);
}
