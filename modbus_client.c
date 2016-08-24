/*
  Check http://www.modbustools.com/modbus.html for the request and response of Modbus RTU frames.
  Be careful that some numbers on this page are not correct.
*/

#include <Python.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "modbus.h"

static PyObject *ModbusClientError = NULL;
static modbus_t *m_modbus = NULL;

static PyObject *
modbus_client_connect(PyObject *self, PyObject *args)
{
    const char *port = NULL;
    int baud = 115200, dataBits = 8, stopBits = 1;
    char parity = 'N';
    uint8_t rsp[1024];

    if (!PyArg_ParseTuple(args, "siici", &port, &baud, &dataBits, &parity, &stopBits))
        return NULL;

    if(m_modbus) {
        modbus_close(m_modbus);
        modbus_free(m_modbus);
        m_modbus = NULL;
    }

    m_modbus = modbus_new_rtu(port, baud, parity, dataBits, stopBits);
    
    if(m_modbus && modbus_connect(m_modbus) == -1) {
        PyErr_SetString(ModbusClientError, "Connection failed");
        modbus_close(m_modbus);
        modbus_free(m_modbus);
        m_modbus = NULL;
        Py_RETURN_FALSE;
    }
    else {
        // modbus_set_debug(m_modbus, 1);
        _modbus_rtu_recv(m_modbus, rsp, 1024); // Clean remaining data in the bus
        usleep(100000);
        modbus_flush(m_modbus); //flush data
        Py_RETURN_TRUE;
    }
}

static PyObject *
modbus_client_flush(PyObject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    if(!m_modbus) {
        PyErr_SetString(ModbusClientError, "Not connected");
        return NULL;
    }

    modbus_flush(m_modbus); // Flush unsent data
    uint8_t rsp[1024];
    _modbus_rtu_recv(m_modbus, rsp, 1024); // Clean remaining data in the bus
    usleep(100000);
    Py_RETURN_NONE;
}

static PyObject *
modbus_client_read_holding_registers(PyObject *self, PyObject *args)
{
    int ret = -1; //return value of modbus functions

    int slave = 1, startAddress = 0, noOfItems = 0;
    if (!PyArg_ParseTuple(args, "iii", &slave, &startAddress, &noOfItems))
        return NULL;

    if(!m_modbus) {
        PyErr_SetString(ModbusClientError, "Not connected");
        return NULL;
    }

    if (noOfItems > (256-5)/2) {
        PyErr_SetString(ModbusClientError, "Requesting too many registers");
        return NULL;
    }

    uint16_t dest16[128];
    memset(dest16, 0, 128);

    modbus_set_slave(m_modbus, slave);
    ret = modbus_read_registers(m_modbus, startAddress, noOfItems, dest16);
    
    if (ret == noOfItems) {
        int i = 0;
        PyObject *l = PyTuple_New(noOfItems);
        if (l == NULL) {
            PyErr_SetString(ModbusClientError, "Internal error when reading");
            return NULL;
        }

        for (i = 0; i < noOfItems; i++) {
            PyTuple_SetItem(l, i, PyInt_FromLong(dest16[i]));
        }
        return l;
    }
    else {
        modbus_flush(m_modbus); //flush data
        if(ret < 0) {
            PyErr_SetString(ModbusClientError, "Slave threw exception");
            return NULL;
        }
        else {
            PyErr_SetString(ModbusClientError, "Number of registers returned does not match");
            return NULL;
        }
    }
}

static PyObject *
modbus_client_write_holding_registers(PyObject *self, PyObject *args)
{
    int ret = -1; //return value of modbus functions

    int slave = 1, startAddress = 0;
    PyObject * listObj;
    if (!PyArg_ParseTuple(args, "iiO!", &slave, &startAddress, &PyList_Type, &listObj))
        return NULL;

    if(!m_modbus) {
        PyErr_SetString(ModbusClientError, "Not connected");
        return NULL;
    }

    int noOfItems = PyList_Size(listObj);
    if (noOfItems > (256-9)/2) {
        PyErr_SetString(ModbusClientError, "Too many register values");
        return NULL;
    }

    uint16_t src16[128];
    memset(src16, 0, 128);
    long asLong = -1;

    int i = 0;
    for (i = 0; i < noOfItems; i++) {
        asLong = PyInt_AsLong(PyList_GetItem(listObj, i));
        if (asLong < 0 || asLong >= 65536) {
            // If PyInt_AsLong() failed, it will return -1
            PyErr_SetString(ModbusClientError, "Invalid register values");
            return NULL;
        }
        else {
            src16[i] = (uint16_t) asLong;
        }
    }

    modbus_set_slave(m_modbus, slave);
    ret = modbus_write_registers(m_modbus, startAddress, noOfItems, src16);

    if (ret == noOfItems) {
        return Py_BuildValue("i", noOfItems);
    }
    else {
        modbus_flush(m_modbus); //flush data
        if(ret < 0) {
            PyErr_SetString(ModbusClientError, "Slave threw exception");
            return NULL;
        }
        else {
            PyErr_SetString(ModbusClientError, "Number of registers returned does not match number of registers requested");
            return NULL;
        }
    }
}

static PyObject *
modbus_client_read_input_registers(PyObject *self, PyObject *args)
{
    int ret = -1; //return value of modbus functions

    int slave = 1, startAddress = 0, noOfItems = 0;
    if (!PyArg_ParseTuple(args, "iii", &slave, &startAddress, &noOfItems))
        return NULL;

    if(!m_modbus) {
        PyErr_SetString(ModbusClientError, "Not connected");
        return NULL;
    }

    uint16_t dest16[128];
    memset(dest16, 0, 128);

    modbus_set_slave(m_modbus, slave);
    ret = modbus_read_input_registers(m_modbus, startAddress, noOfItems, dest16);
    
    if (ret == noOfItems) {
        int i = 0;
        PyObject *l = PyTuple_New(noOfItems);
        if (l == NULL) {
            PyErr_SetString(ModbusClientError, "Internal error when reading");
            return NULL;
        }

        for (i = 0; i < noOfItems; i++) {
            PyTuple_SetItem(l, i, PyInt_FromLong(dest16[i]));
        }
        return l;
    }
    else {
        modbus_flush(m_modbus); //flush data
        if(ret < 0) {
            PyErr_SetString(ModbusClientError, "Slave threw exception");
            return NULL;
        }
        else {
            PyErr_SetString(ModbusClientError, "Number of registers returned does not match");
            return NULL;
        }
    }
}

static PyObject *
modbus_client_disconnect(PyObject *self, PyObject *args)
{ 
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    if (m_modbus) {
        modbus_close(m_modbus);
        modbus_free(m_modbus);
        m_modbus = NULL;
    }

    Py_RETURN_TRUE;
}

static PyMethodDef ModbusClientMethods[] = {
    {"connect",  modbus_client_connect, METH_VARARGS,
     "Connect to a serial bus:\n"
     "    connect(port, baud, start_bits, parity, stop_bits)\n\n"
     "Example:\n" 
     "    connect(\"/dev/tty.usbmodem1412\", 115200, 8, 'N', 1)"},
    {"read_holding_registers",  modbus_client_read_holding_registers, METH_VARARGS,
     "Read holding registers:\n"
     "    read_holding_registers(unit_addr, starting_reg_addr, num_regs)\n\n"
     "Example:\n"
     "    read_holding_registers(0x01, 0x0000, 11)"},
    {"write_holding_registers",  modbus_client_write_holding_registers, METH_VARARGS,
     "Write holding registers:\n"
     "    write_holding_registers(unit_addr, starting_reg_addr, list_of_reg_values)\n\n"
     "Example:\n"
     "    write_holding_registers(0x01, 0x0000, [12, 0, 0, 0, 1, 1, 1, 1])"},
    {"read_input_registers",  modbus_client_read_input_registers, METH_VARARGS,
     "Read input registers:\n"
     "    read_input_registers(unit_addr, starting_reg_addr, num_regs)\n\n"
     "Example:\n"
     "    read_input_registers(0x01, 0x0000, 12)"},
    {"disconnect",  modbus_client_disconnect, METH_VARARGS,
     "Disconnect from a serial bus:\n"
     "    disconnect()"},
    {"_flush",  modbus_client_flush, METH_VARARGS,
     "Clean the unread and unsent data:\n"
     "    _flush()"},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};


PyMODINIT_FUNC
initmodbus_client(void)
{
    PyObject *m;

    m = Py_InitModule("modbus_client", ModbusClientMethods);
    if (m == NULL)
        return;

    ModbusClientError = PyErr_NewException("modbus_client.error", NULL, NULL);
    Py_INCREF(ModbusClientError);
    PyModule_AddObject(m, "error", ModbusClientError);
}
