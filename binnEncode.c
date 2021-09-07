#include <string.h>
#include <stdint.h>

#include "mex.h"

#include "binn.h"

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof((arr)[0]))

typedef struct {
    /**
     * Condition whether encoder supports encoding given object
     * @param obj
     */
    bool ( *predicate)(const mxArray *obj);

    /**
     * Function that should encode given object
     * @param obj
     */
    binn *( *encoder)(const mxArray *obj);
} Encoder;

// map for easier conversion from matlab numeric to binn values
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

static binn *encode(const mxArray *obj);

static binn *encodeStruct(const mxArray *obj);

static binn *encodeCell(const mxArray *obj);

static binn *encodeNumeric(const mxArray *obj);

static binn *encodeString(const mxArray *obj);

static binn *encodeScalar(const mxArray *obj);

static binn *encodeArray(const mxArray *obj);

static Encoder encoders[] = {
        {mxIsStruct,  encodeStruct},
        {mxIsCell,    encodeCell},
        {mxIsNumeric, encodeNumeric},
        {mxIsChar,    encodeString},
};

void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[]) {
    // check number of arguments
    if (nrhs != 1) {
        mexErrMsgTxt("One input is required.");
    } else if (nlhs > 1) {
        mexErrMsgTxt("Too many output arguments.");
    }

    const mxArray *original = prhs[0];

    binn *obj = encode(original);

    if (obj == NULL) {
        mexErrMsgTxt("Can't encode provided value");
    }
    if (binn_size(obj) == 0) {
        // scalar values will have 0 size, encode them as arrays
        binn_free(obj);
        obj = encodeArray(original);
    }


    // convert encoded data to uint8 array
    mwSize dims[] = {1, binn_size(obj)};
    plhs[0] = mxCreateNumericArray(2, dims, mxUINT8_CLASS, mxREAL);
    memcpy(mxGetPr(plhs[0]), binn_ptr(obj), dims[1]);

    binn_free(obj);
}

static binn *encode(const mxArray *obj) {
    //TODO error checking
    if (mxIsComplex(obj)) {
        mexWarnMsgTxt("Complex data is not supported");
        return NULL;
    } else if (mxIsSparse(obj)) {
        mexWarnMsgTxt("Sparse data is not supported");
        return NULL;
    }

    for (int i = 0; i < ARRAY_SIZE(encoders); ++i) {
        if (encoders[i].predicate(obj)) {
            return encoders[i].encoder(obj);
        }
    }

    return NULL;
}

static binn *encodeStruct(const mxArray *obj) {
    binn *encoded = binn_object();

    int fieldNum = mxGetNumberOfFields(obj);
    //TODO check if struct is empty?
    for (int i = 0; i < fieldNum; ++i) {
        mxArray *value = mxGetFieldByNumber(obj, 0, i);
        const char *name = mxGetFieldNameByNumber(obj, i);

        binn *bval = encode(value);

        if (bval != NULL) {
            binn_object_set_value(encoded, name, bval);
            binn_free(bval);
        } else {
            mexPrintf("Skipped field '%s' with unknown or invalid value\n", name);
        }
    }
    return encoded;
}

static binn *encodeCell(const mxArray *obj) {
    mwSize ndims = mxGetNumberOfDimensions(obj);
    if (ndims > 2) {
        mexWarnMsgTxt("Multidimensional data is not supported");
        return NULL;
    }
    size_t rows = mxGetM(obj);
    size_t cols = mxGetN(obj);
    if (rows != 1 && cols != 1) {
        mexWarnMsgTxt("Only rows (1xN) or vectors (Nx1) are supported");
        return NULL;
    }
    if (rows > cols) {
        cols = rows;
    }

    binn *list = binn_list();

    for (int i = 0; i < cols; ++i) {
        binn *item = encode(mxGetCell(obj, i));
        if (item == NULL) {
            mexPrintf("Skipped element at index %d in cell array\n", i);
        } else {
            binn_list_add_value(list, item);
            binn_free(item);
        }
    }

    return list;
}

static binn *encodeNumeric(const mxArray *obj) {
    mwSize ndims = mxGetNumberOfDimensions(obj);
    if (ndims > 2) {
        mexWarnMsgTxt("Multidimensional data is not supported");
        return NULL;
    }
    size_t rows = mxGetM(obj);
    size_t cols = mxGetN(obj);
    if (rows > 1) {
        mexWarnMsgTxt("Only row-vectors are supported");
        return NULL;
    }

    if (cols == 1) {
        return encodeScalar(obj);
    } else {
        return encodeArray(obj);
    }
}

static binn *encodeString(const mxArray *obj) {
    char *str = mxArrayToString(obj);
    binn *encoded = binn_string(str, BINN_TRANSIENT);
    mxFree(str);
    return encoded;
}

static binn *encodeScalar(const mxArray *obj) {
    mxClassID classId = mxGetClassID(obj);
    int type = 0;

    for (int i = 0; i < ARRAY_SIZE(typeMap); ++i) {
        if (typeMap[i][1] == classId) {
            type = typeMap[i][0];
            break;
        }
    }

    if (type != 0) {
        return binn_value(type, mxGetPr(obj), 0, NULL);
    }

    return NULL;
}

static binn *encodeArray(const mxArray *obj) {
    mxClassID classId = mxGetClassID(obj);
    int type = 0;

    for (int i = 0; i < ARRAY_SIZE(typeMap); ++i) {
        if (typeMap[i][1] == classId) {
            type = typeMap[i][0];
            break;
        }
    }

    if (type == 0) {
        return NULL;
    }

    binn *list = binn_list();

    uint8_t *data = (uint8_t *) mxGetPr(obj);

    size_t elemSize = mxGetElementSize(obj);

    size_t cols = mxGetN(obj);
    for (int i = 0; i < cols; ++i) {
        binn_list_add(list, type, &data[i * elemSize], 0);
    }

    return list;
}
