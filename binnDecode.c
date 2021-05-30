#include <string.h>
#include <stdint.h>

#include "mex.h"

#include "binn.h"

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

mxArray *decode(binn *obj) {
    //TODO error checking

    if (obj->type == BINN_OBJECT) {
        return decodeObject(obj);
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
