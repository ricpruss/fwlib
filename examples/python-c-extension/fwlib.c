#include <Python.h>
#include "fwlib32.h"
#include <stdlib.h> // Added for malloc/free
#include <string.h> // Added for memcpy/memset

#ifdef _MSC_VER
#pragma pack(push, 4)
#endif

#define MACHINE_PORT_DEFAULT 8193
#define TIMEOUT_DEFAULT 10

typedef struct {
    PyObject_HEAD
    unsigned short libh;
    int connected;
} Context;

#ifndef _WIN32
int cnc_startup() {
    return cnc_startupprocess(0, "focas.log");
}

void cnc_shutdown() {
    cnc_exitprocess();
}
#endif

static PyObject* Context_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
    Context* self;
    self = (Context*) type->tp_alloc(type, 0);
    if (self != NULL) {
        self->libh = 0;
        self->connected = 0;
    }
    return (PyObject*) self;
}

static int Context_init(Context* self, PyObject* args, PyObject* kwds) {
    const char* host = "127.0.0.1";
    int port = MACHINE_PORT_DEFAULT;
    int timeout = TIMEOUT_DEFAULT;
    int ret;

    static char* kwlist[] = {"host", "port", "timeout", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|sii", kwlist, &host, &port, &timeout)) {
        return -1;
    }

#ifndef _WIN32
    ret = cnc_startup();
    if (ret != EW_OK) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to start FANUC process.");
        return -1;
    }
#endif

    ret = cnc_allclibhndl3(host, port, timeout, &self->libh);
    if (ret != EW_OK) {
        PyErr_Format(PyExc_ConnectionError, "Failed to connect to CNC: %d", ret);
        return -1;
    }
    self->connected = 1;

    return 0;
}

static void Context_dealloc(Context* self) {
    if (self->connected) {
        cnc_freelibhndl(self->libh);
        self->connected = 0;
    }

#ifndef _WIN32
    cnc_shutdown();
#endif

    Py_TYPE(self)->tp_free((PyObject*) self);
}

static PyObject* Context_read_id(Context* self, PyObject* Py_UNUSED(ignored)) {
    uint32_t cnc_ids[4] = {0};
    char cnc_id[40] = "";
    int ret;

    ret = cnc_rdcncid(self->libh, (unsigned long*) cnc_ids);
    if (ret != EW_OK) {
        PyErr_Format(PyExc_RuntimeError, "Failed to read CNC ID: %d", ret);
        return NULL;
    }

    snprintf(cnc_id, sizeof(cnc_id), "%08x-%08x-%08x-%08x",
             cnc_ids[0], cnc_ids[1], cnc_ids[2], cnc_ids[3]);

    return PyUnicode_FromString(cnc_id);
}

static PyObject* Context_read_status(Context* self, PyObject* Py_UNUSED(ignored)) {
    ODBST status;
    int ret;

    ret = cnc_statinfo(self->libh, &status);
    if (ret != EW_OK) {
        PyErr_Format(PyExc_RuntimeError, "Failed to read status info: %d", ret);
        return NULL;
    }

    PyObject* dict = PyDict_New();
    if (!dict) return NULL;

    // Add all status information to dictionary
    PyDict_SetItemString(dict, "aut", PyLong_FromLong(status.aut));
    PyDict_SetItemString(dict, "run", PyLong_FromLong(status.run));
    PyDict_SetItemString(dict, "motion", PyLong_FromLong(status.motion));
    PyDict_SetItemString(dict, "mstb", PyLong_FromLong(status.mstb));
    PyDict_SetItemString(dict, "emergency", PyLong_FromLong(status.emergency));
    PyDict_SetItemString(dict, "alarm", PyLong_FromLong(status.alarm));
    PyDict_SetItemString(dict, "edit", PyLong_FromLong(status.edit));
    
    // Add mode information
    PyDict_SetItemString(dict, "tmmode", PyLong_FromLong(status.tmmode)); // T/M mode
    PyDict_SetItemString(dict, "hdck", PyLong_FromLong(status.hdck)); // Handle retrace status
    
    // Derive mode information from available fields
    // T/M mode 1 is MDI
    PyDict_SetItemString(dict, "mdi", PyLong_FromLong(status.tmmode == 1));
    
    // Auto mode 1 is AUTO
    PyDict_SetItemString(dict, "auto", PyLong_FromLong(status.aut == 1));
    
    // For JOG mode, we need to check if we're in manual mode and not in MDI
    // This is an approximation since we don't have direct access to manual mode
    PyDict_SetItemString(dict, "jog", PyLong_FromLong(status.tmmode != 1 && status.aut != 1));

    return dict;
}

static PyObject* Context_read_position(Context* self, PyObject* Py_UNUSED(ignored)) {
    ODBPOS pos;
    short s4 = 4;  // Number of axes to read
    int ret;

    memset(&pos, 0, sizeof(ODBPOS));  // Initialize the structure to zero

    ret = cnc_rdposition(self->libh, -1, &s4, &pos);
    if (ret != EW_OK) {
        PyErr_Format(PyExc_RuntimeError, "Failed to read position: %d", ret);
        return NULL;
    }

    PyObject* dict = PyDict_New();
    if (!dict) {
        return NULL;
    }

    // Create Python objects for absolute position
    PyObject* abs_data = PyLong_FromLong(pos.abs.data);
    if (!abs_data) {
        Py_DECREF(dict);
        return NULL;
    }
    PyDict_SetItemString(dict, "abs_pos", abs_data);
    Py_DECREF(abs_data);

    // Create Python objects for machine position
    PyObject* mach_data = PyLong_FromLong(pos.mach.data);
    if (!mach_data) {
        Py_DECREF(dict);
        return NULL;
    }
    PyDict_SetItemString(dict, "mchn_pos", mach_data);
    Py_DECREF(mach_data);

    // Create Python objects for relative position
    PyObject* rel_data = PyLong_FromLong(pos.rel.data);
    if (!rel_data) {
        Py_DECREF(dict);
        return NULL;
    }
    PyDict_SetItemString(dict, "rel_pos", rel_data);
    Py_DECREF(rel_data);

    // Create Python objects for distance to go
    PyObject* dist_data = PyLong_FromLong(pos.dist.data);
    if (!dist_data) {
        Py_DECREF(dict);
        return NULL;
    }
    PyDict_SetItemString(dict, "dist", dist_data);
    Py_DECREF(dist_data);

    return dict;
}

static PyObject* Context_read_spindle(Context* self, PyObject* Py_UNUSED(ignored)) {
    ODBSPEED speed;
    int ret;

    ret = cnc_rdspeed(self->libh, -1, &speed);
    if (ret != EW_OK) {
        PyErr_Format(PyExc_RuntimeError, "Failed to read spindle speed: %d", ret);
        return NULL;
    }

    PyObject* dict = PyDict_New();
    if (!dict) return NULL;

    // Add spindle speed information to dictionary
    PyDict_SetItemString(dict, "feed", PyLong_FromLong(speed.actf.data));
    PyDict_SetItemString(dict, "spindle", PyLong_FromLong(speed.acts.data));

    return dict;
}

static PyObject* Context_read_pmc(Context* self, PyObject* args) {
    short adr_type, data_type;
    unsigned short start_num, end_num;
    
    if (!PyArg_ParseTuple(args, "hhHH", &adr_type, &data_type, &start_num, &end_num)) {
        return NULL;
    }
    
    // Calculate length based on data type and range
    unsigned short data_count = end_num - start_num + 1;
    unsigned short length;
    
    // Calculate buffer size based on data type
    switch (data_type) {
        case 0: // Byte type
            length = 8 + data_count;
            break;
        case 1: // Word type
            length = 8 + (data_count * 2);
            break;
        case 2: // Long type
            length = 8 + (data_count * 4);
            break;
        case 4: // Float type (32-bit)
            length = 8 + (data_count * 4);
            break;
        case 5: // Double type (64-bit)
            length = 8 + (data_count * 8);
            break;
        default:
            PyErr_SetString(PyExc_ValueError, "Invalid data_type");
            return NULL;
    }
    
    // Allocate memory for IODBPMC structure
    IODBPMC* buf = (IODBPMC*)malloc(length);
    if (!buf) {
        PyErr_SetString(PyExc_MemoryError, "Failed to allocate memory for PMC data");
        return NULL;
    }
    
    // Read PMC data
    int ret = pmc_rdpmcrng(self->libh, adr_type, data_type, start_num, end_num, length, buf);
    if (ret != EW_OK) {
        free(buf);
        PyErr_Format(PyExc_RuntimeError, "Failed to read PMC data: %d", ret);
        return NULL;
    }
    
    // Create result list
    PyObject* result_list = PyList_New(data_count);
    if (!result_list) {
        free(buf);
        return NULL;
    }
    
    // Extract data based on data type
    for (unsigned short i = 0; i < data_count; i++) {
        PyObject* value = NULL;
        
        switch (data_type) {
            case 0: // Byte type
                value = PyLong_FromLong((long)(buf->u.cdata[i]));
                break;
            case 1: // Word type
                value = PyLong_FromLong((long)(buf->u.idata[i]));
                break;
            case 2: // Long type
                value = PyLong_FromLong(buf->u.ldata[i]);
                break;
            case 4: // Float type
                value = PyFloat_FromDouble((double)(buf->u.fdata[i]));
                break;
            case 5: // Double type
                value = PyFloat_FromDouble(buf->u.dfdata[i]);
                break;
        }
        
        if (value) {
            PyList_SetItem(result_list, i, value);
        } else {
            Py_DECREF(result_list);
            free(buf);
            return NULL;
        }
    }
    
    free(buf);
    return result_list;
}

static PyObject* Context_read_pmc_bit(Context* self, PyObject* args) {
    short adr_type;
    unsigned short adr_num;
    short bit_pos;
    
    if (!PyArg_ParseTuple(args, "hHh", &adr_type, &adr_num, &bit_pos)) {
        return NULL;
    }
    
    // Check bit position (0-7)
    if (bit_pos < 0 || bit_pos > 7) {
        PyErr_SetString(PyExc_ValueError, "Bit position must be between 0 and 7");
        return NULL;
    }
    
    // Always use byte type (0) for bit access
    short data_type = 0;
    unsigned short length = 8 + 1;  // Header (8) + 1 byte of data
    
    // Allocate memory for IODBPMC structure
    IODBPMC* buf = (IODBPMC*)malloc(length);
    if (!buf) {
        PyErr_SetString(PyExc_MemoryError, "Failed to allocate memory for PMC data");
        return NULL;
    }
    
    // Read PMC data (single byte)
    int ret = pmc_rdpmcrng(self->libh, adr_type, data_type, adr_num, adr_num, length, buf);
    if (ret != EW_OK) {
        free(buf);
        PyErr_Format(PyExc_RuntimeError, "Failed to read PMC data: %d", ret);
        return NULL;
    }
    
    // Extract the bit value
    int bit_value = (buf->u.cdata[0] >> bit_pos) & 0x01;
    
    free(buf);
    return PyBool_FromLong(bit_value);
}

static PyObject* Context_write_pmc(Context* self, PyObject* args) {
    short adr_type, data_type;
    unsigned short start_num, end_num;
    PyObject* data_list; // Python list/tuple containing data to write

    // Parse arguments: adr_type, data_type, start_num, end_num, data_list
    if (!PyArg_ParseTuple(args, "hhHHO", &adr_type, &data_type, &start_num, &end_num, &data_list)) {
        return NULL;
    }

    // Validate input data list
    if (!PyList_Check(data_list) && !PyTuple_Check(data_list)) {
        PyErr_SetString(PyExc_TypeError, "Data argument must be a list or tuple");
        return NULL;
    }

    Py_ssize_t data_count_py = PySequence_Size(data_list);
    unsigned short data_count = end_num - start_num + 1;

    if (data_count_py != data_count) {
         PyErr_Format(PyExc_ValueError, "Data list size (%zd) does not match the specified range size (%u)", data_count_py, data_count);
         return NULL;
    }

    // Calculate length based on data type and range
    unsigned short length;
    size_t element_size;

    switch (data_type) {
        case 0: // Byte type
            length = 8 + data_count;
            element_size = sizeof(char);
            break;
        case 1: // Word type
            length = 8 + (data_count * 2);
            element_size = sizeof(short);
            break;
        case 2: // Long type
            length = 8 + (data_count * 4);
            element_size = sizeof(long);
            break;
        case 4: // Float type (32-bit)
            length = 8 + (data_count * 4);
            element_size = sizeof(float);
            break;
        case 5: // Double type (64-bit)
            length = 8 + (data_count * 8);
            element_size = sizeof(double);
            break;
        default:
            PyErr_SetString(PyExc_ValueError, "Invalid data_type");
            return NULL;
    }

    // Allocate memory for IODBPMC structure
    IODBPMC* buf = (IODBPMC*)malloc(length);
    if (!buf) {
        PyErr_SetString(PyExc_MemoryError, "Failed to allocate memory for PMC data");
        return NULL;
    }
    memset(buf, 0, length); // Initialize buffer

    // Populate the buffer with data from the Python list
    buf->type_a = adr_type;
    buf->type_d = data_type;
    buf->datano_s = start_num;
    buf->datano_e = end_num;

    for (unsigned short i = 0; i < data_count; i++) {
        PyObject* item = PySequence_GetItem(data_list, i);
        if (!item) {
            free(buf);
            // Error already set by PySequence_GetItem
            return NULL;
        }

        long long_val;
        double double_val;

        switch (data_type) {
            case 0: // Byte type
                if (!PyLong_Check(item)) { Py_DECREF(item); free(buf); PyErr_SetString(PyExc_TypeError, "Expected int for byte type"); return NULL; }
                long_val = PyLong_AsLong(item);
                if (long_val < 0 || long_val > 255) { Py_DECREF(item); free(buf); PyErr_SetString(PyExc_ValueError, "Byte value out of range (0-255)"); return NULL; }
                buf->u.cdata[i] = (char)long_val;
                break;
            case 1: // Word type
                if (!PyLong_Check(item)) { Py_DECREF(item); free(buf); PyErr_SetString(PyExc_TypeError, "Expected int for word type"); return NULL; }
                long_val = PyLong_AsLong(item);
                 // Check range for short if necessary, depends on signed/unsigned nature of PMC WORD
                // Assuming signed short for now: SHRT_MIN to SHRT_MAX
                if (long_val < SHRT_MIN || long_val > SHRT_MAX) { Py_DECREF(item); free(buf); PyErr_SetString(PyExc_ValueError, "Word value out of range for short"); return NULL; }
                buf->u.idata[i] = (short)long_val;
                break;
            case 2: // Long type
                if (!PyLong_Check(item)) { Py_DECREF(item); free(buf); PyErr_SetString(PyExc_TypeError, "Expected int for long type"); return NULL; }
                long_val = PyLong_AsLong(item);
                // Check range for long if necessary: LONG_MIN to LONG_MAX
                 if (long_val < LONG_MIN || long_val > LONG_MAX) { Py_DECREF(item); free(buf); PyErr_SetString(PyExc_ValueError, "Long value out of range for long"); return NULL; }
                buf->u.ldata[i] = (long)long_val;
                break;
            case 4: // Float type
                if (!PyFloat_Check(item) && !PyLong_Check(item)) { Py_DECREF(item); free(buf); PyErr_SetString(PyExc_TypeError, "Expected float or int for float type"); return NULL; }
                double_val = PyFloat_AsDouble(item);
                if (PyErr_Occurred()) { Py_DECREF(item); free(buf); return NULL; } // Handle conversion error
                buf->u.fdata[i] = (float)double_val;
                break;
            case 5: // Double type
                 if (!PyFloat_Check(item) && !PyLong_Check(item)) { Py_DECREF(item); free(buf); PyErr_SetString(PyExc_TypeError, "Expected float or int for double type"); return NULL; }
                double_val = PyFloat_AsDouble(item);
                 if (PyErr_Occurred()) { Py_DECREF(item); free(buf); return NULL; } // Handle conversion error
                buf->u.dfdata[i] = double_val;
                break;
        }
        Py_DECREF(item); // Decrement reference count for the item
    }

    // Write PMC data
    int ret = pmc_wrpmcrng(self->libh, length, buf);
    free(buf); // Free memory after the call

    if (ret != EW_OK) {
        PyErr_Format(PyExc_RuntimeError, "Failed to write PMC data: %d", ret);
        return NULL;
    }

    Py_RETURN_NONE; // Indicate success
}

static PyObject* Context_wrmdiprog(Context* self, PyObject* args) {
    int length;
    const char* command;
    int ret;

    if (!PyArg_ParseTuple(args, "is", &length, &command)) {
        return NULL;
    }

    ret = cnc_wrmdiprog(self->libh, length, (char*)command);
    return PyLong_FromLong(ret);
}

static PyObject* Context_wrjogmdi(Context* self, PyObject* args) {
    const char* command;
    int ret;

    if (!PyArg_ParseTuple(args, "s", &command)) {
        return NULL;
    }

    ret = cnc_wrjogmdi(self->libh, (char*)command);
    if (ret != EW_OK) {
        PyErr_Format(PyExc_RuntimeError, "Failed to write JOG MDI command: %d", ret);
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject* Context_set_mode(Context* self, PyObject* args) {
    const char* mode;
    int ret;
    IODBSGNL sgnl;

    if (!PyArg_ParseTuple(args, "s", &mode)) {
        return NULL;
    }

    // Initialize the signal structure
    memset(&sgnl, 0, sizeof(IODBSGNL));
    sgnl.datano = 0;
    sgnl.type = 0;

    // Set the mode based on the input string
    if (strcmp(mode, "mdi") == 0) {
        sgnl.mode = 1;  // MDI mode
    } else if (strcmp(mode, "auto") == 0) {
        sgnl.mode = 2;  // AUTO mode
    } else if (strcmp(mode, "jog") == 0) {
        sgnl.mode = 3;  // JOG mode
    } else {
        PyErr_SetString(PyExc_ValueError, "Invalid mode. Must be 'mdi', 'auto', or 'jog'");
        return NULL;
    }

    ret = cnc_wropnlsgnl(self->libh, &sgnl);
    if (ret != EW_OK) {
        PyErr_Format(PyExc_RuntimeError, "Failed to set operation mode: %d", ret);
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject* Context_enter(PyObject* self) {
    Py_INCREF(self);
    return self;
}

static PyObject* Context_exit(Context* self, PyObject* exc_type, PyObject* exc_value, PyObject* traceback) {
    if (self->connected) {
        cnc_freelibhndl(self->libh);
        self->connected = 0;
    }

#ifndef _WIN32
    cnc_shutdown();
#endif

    Py_RETURN_NONE;
}

static PyObject* Context_cycle_start(Context* self, PyObject* Py_UNUSED(ignored)) {
    int ret;

    ret = cnc_start(self->libh);
    if (ret != EW_OK) {
        PyErr_Format(PyExc_RuntimeError, "Failed to send cycle start command: %d", ret);
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject* Context_read_program_number(Context* self, PyObject* Py_UNUSED(ignored)) {
    ODBPRO prog_num; // Use the typedef ODBPRO which handles ONO8D macro
    int ret;

    memset(&prog_num, 0, sizeof(ODBPRO));

    ret = cnc_rdprgnum(self->libh, &prog_num);
    if (ret != EW_OK) {
        PyErr_Format(PyExc_RuntimeError, "Failed to read program number: %d", ret);
        return NULL;
    }

    PyObject* dict = PyDict_New();
    if (!dict) return NULL;

    PyDict_SetItemString(dict, "running_program", PyLong_FromLong(prog_num.data));
    PyDict_SetItemString(dict, "main_program", PyLong_FromLong(prog_num.mdata));

    // Check PyDict_SetItemString errors (though unlikely for PyLong_FromLong)
    if (PyErr_Occurred()) {
        Py_DECREF(dict);
        return NULL;
    }

    return dict;
}

static PyObject* Context_read_main_program_path(Context* self, PyObject* Py_UNUSED(ignored)) {
    // ODBMAIN struct indicates max path size is 256
    char path_buffer[256]; 
    int ret;

    memset(path_buffer, 0, sizeof(path_buffer)); // Zero out the buffer

    // cnc_pdf_rdmain expects a char* buffer, not an ODBMAIN struct pointer
    ret = cnc_pdf_rdmain(self->libh, path_buffer);
    if (ret != EW_OK) {
        PyErr_Format(PyExc_RuntimeError, "Failed to read main program path: %d", ret);
        return NULL;
    }

    // Ensure null termination just in case, though memset should handle it.
    path_buffer[sizeof(path_buffer) - 1] = '\0';

    // Return just the path string
    return PyUnicode_FromString(path_buffer);
}

static PyObject* Context_select_main_program(Context* self, PyObject* args) {
    const char* path;
    int ret;

    // Expect a single string argument for the path
    if (!PyArg_ParseTuple(args, "s", &path)) {
        return NULL; // Error parsing arguments
    }

    ret = cnc_pdf_slctmain(self->libh, (char*)path); // Cast needed as API expects char*
    if (ret != EW_OK) {
        PyErr_Format(PyExc_RuntimeError, "Failed to select main program '%s': %d", path, ret);
        return NULL;
    }

    Py_RETURN_NONE; // Indicate success
}

static PyObject* Context_get_detailed_error(Context* self, PyObject* Py_UNUSED(ignored)) {
    ODBERR err_info;
    int ret;

    memset(&err_info, 0, sizeof(ODBERR));

    // Call the FOCAS function to get detailed error info
    ret = cnc_getdtailerr(self->libh, &err_info);
    if (ret != EW_OK) {
        // This function itself failed, perhaps invalid handle or communication issue
        PyErr_Format(PyExc_RuntimeError, "Failed to execute cnc_getdtailerr: %d", ret);
        return NULL;
    }

    // Create a dictionary to return the detailed error info
    PyObject* dict = PyDict_New();
    if (!dict) return NULL; // Memory allocation failed

    PyDict_SetItemString(dict, "detail_error_code", PyLong_FromLong(err_info.err_no));
    PyDict_SetItemString(dict, "detail_error_data", PyLong_FromLong(err_info.err_dtno));

    // Check for errors during dictionary population
    if (PyErr_Occurred()) {
        Py_DECREF(dict);
        return NULL;
    }

    return dict;
}

static PyMethodDef Context_methods[] = {
    {"read_id", (PyCFunction)Context_read_id, METH_NOARGS, "Read CNC ID"},
    {"read_status", (PyCFunction)Context_read_status, METH_NOARGS, "Read CNC status"},
    {"read_position", (PyCFunction)Context_read_position, METH_NOARGS, "Read CNC position"},
    {"read_spindle", (PyCFunction)Context_read_spindle, METH_NOARGS, "Read spindle information"},
    {"read_pmc", (PyCFunction)Context_read_pmc, METH_VARARGS, "Read PMC data"},
    {"read_pmc_bit", (PyCFunction)Context_read_pmc_bit, METH_VARARGS, "Read PMC bit"},
    {"write_pmc", (PyCFunction)Context_write_pmc, METH_VARARGS, "Write PMC data"},
    {"read_program_number", (PyCFunction)Context_read_program_number, METH_NOARGS, "Read running and main program numbers"},
    {"read_main_program_path", (PyCFunction)Context_read_main_program_path, METH_NOARGS, "Read current main program path"},
    {"select_main_program", (PyCFunction)Context_select_main_program, METH_VARARGS, "Select the main program by path"},
    {"get_detailed_error", (PyCFunction)Context_get_detailed_error, METH_NOARGS, "Get detailed error info for the last failed operation"},
    {"wrmdiprog", (PyCFunction)Context_wrmdiprog, METH_VARARGS, "Write MDI program"},
    {"wrjogmdi", (PyCFunction)Context_wrjogmdi, METH_VARARGS, "Write JOG MDI command"},
    {"set_mode", (PyCFunction)Context_set_mode, METH_VARARGS, "Set operation mode (mdi/auto/jog)"},
    {"cycle_start", (PyCFunction)Context_cycle_start, METH_NOARGS, "Send cycle start command to CNC"},
    {"__enter__", (PyCFunction)Context_enter, METH_NOARGS, "Enter the context."},
    {"__exit__", (PyCFunction)Context_exit, METH_VARARGS, "Exit the context."},
    {NULL}  /* Sentinel */
};

static PyTypeObject ContextType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "fwlib.Context",
    .tp_doc = "FANUC Context Manager",
    .tp_basicsize = sizeof(Context),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = Context_new,
    .tp_init = (initproc) Context_init,
    .tp_dealloc = (destructor) Context_dealloc,
    .tp_methods = Context_methods,
};

static PyModuleDef fwlibmodule = {
    PyModuleDef_HEAD_INIT,
    "fwlib",
    "Python wrapper for FANUC fwlib32 library",
    -1,
    NULL, NULL, NULL, NULL, NULL
};

PyMODINIT_FUNC PyInit_fwlib(void) {
    PyObject* m;
    if (PyType_Ready(&ContextType) < 0)
        return NULL;

    m = PyModule_Create(&fwlibmodule);
    if (m == NULL)
        return NULL;

    Py_INCREF(&ContextType);
    if (PyModule_AddObject(m, "Context", (PyObject*) &ContextType) < 0) {
        Py_DECREF(&ContextType);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}

