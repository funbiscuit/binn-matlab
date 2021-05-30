#include <string.h>
#include <stdint.h>

#include "mex.h"

#include "binn.h"

#define TYPE_MAP_LEN (sizeof(typeMap)/sizeof(*typeMap))

// map for easier conversion from matlab numeric to binn values
int typeMap[][2] = {
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

binn *encodeScalar(const mxArray *obj) {
    mxClassID classId = mxGetClassID(obj);
    int type = 0;

    for (int i = 0; i < TYPE_MAP_LEN; ++i) {
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

binn *encodeArray(const mxArray *obj) {
    mxClassID classId = mxGetClassID(obj);
    int type = 0;

    for (int i = 0; i < TYPE_MAP_LEN; ++i) {
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

    int elemSize = mxGetElementSize(obj);

    mwSize cols = mxGetN(obj);
    for (int i = 0; i < cols; ++i) {
        binn_list_add(list, type, &data[i * elemSize], 0);
    }

    return list;
}

binn *encode(const mxArray *obj) {
    //TODO error checking
    binn *encoded = NULL;

    if (mxIsStruct(obj)) {
        encoded = binn_object();

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
    } else if (mxIsNumeric(obj)) {
        if (mxIsComplex(obj)) {
            mexWarnMsgTxt("Complex data is not supported");
            return NULL;
        } else if (mxIsSparse(obj)) {
            mexWarnMsgTxt("Sparse data is not supported");
            return NULL;
        }
        mwSize ndims = mxGetNumberOfDimensions(obj);
        if (ndims > 2) {
            mexWarnMsgTxt("Multidimensional data is not supported");
            return NULL;
        }
        mwSize rows = mxGetM(obj);
        mwSize cols = mxGetN(obj);
        if (rows != 1) {
            mexWarnMsgTxt("Only row-vectors are supported");
            return NULL;
        }

        if (cols == 1) {
            encoded = encodeScalar(obj);
        } else {
            encoded = encodeArray(obj);
        }
    } else if (mxIsChar(obj)) {
        char *str = mxArrayToString(obj);
        encoded = binn_string(str, BINN_TRANSIENT);
        mxFree(str);
    }

    return encoded;
}

void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[]) {
    // check number of arguments
    if (nrhs != 1) {
        mexErrMsgTxt("One input is required.");
    } else if (nlhs > 1) {
        mexErrMsgTxt("Too many output arguments.");
    }

    const mxArray *original = prhs[0];

    if (!mxIsStruct(original)) {
        mexErrMsgTxt("Only structures are supported");
    }

    binn *obj = encode(original);

    if (obj == NULL) {
        mexErrMsgTxt("Can't encode provided value");
    }

    // convert encoded data to uint8 array
    mwSize len = binn_size(obj);
    plhs[0] = mxCreateNumericArray(1, &len, mxUINT8_CLASS, mxREAL);
    memcpy(mxGetPr(plhs[0]), binn_ptr(obj), len);

    binn_free(obj);
}
