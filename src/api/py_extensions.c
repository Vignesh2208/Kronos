#include <Python.h>
#include "Kronos_functions.h"
#include "kronos_utility_functions.h"

static PyObject *py_getCurrentVirtualTime(PyObject *self, PyObject *args) {
  s64 ret;
  ret = getCurrentVirtualTime();
  return Py_BuildValue("L", ret);
}

static PyObject *py_gettime_pid(PyObject *self, PyObject *args) {
  s64 ret;
  int pid;

  if (!PyArg_ParseTuple(args, "i", &pid)) return NULL;

  ret = getCurrentTimePid(pid);
  return Py_BuildValue("L", ret);
}

static PyObject *py_setNetDeviceOwner(PyObject *self, PyObject *args) {
  int tracer_id;
  char *intf_name;
  int ret;

  if (!PyArg_ParseTuple(args, "is", &tracer_id, &intf_name)) return NULL;
  ret = setNetDeviceOwner(tracer_id, intf_name);
  return Py_BuildValue("i", ret);
}

static PyObject *py_stopExp(PyObject *self, PyObject *args) {
  int ret;
  ret = stopExp();
  return Py_BuildValue("i", ret);
}

static PyObject *py_initializeExp(PyObject *self, PyObject *args) {
  int ret;
  int n_tracers;
  if (!PyArg_ParseTuple(args, "i", &n_tracers)) return NULL;

  ret = initializeExp(n_tracers);
  return Py_BuildValue("i", ret);
}

static PyObject *py_initializeVTExp(PyObject *self, PyObject *args) {
  int ret;
  int n_tracers;
  int exp_type;
  int n_timelines;
  if (!PyArg_ParseTuple(args, "iii", &exp_type, &n_timelines, &n_tracers)) 
    return NULL;

  ret = initializeVtExp(exp_type, n_timelines, n_tracers);
  return Py_BuildValue("i", ret);
}


static PyObject *py_progress_by(PyObject *self, PyObject *args) {
  s64 duration;
  int num_rounds;
  int ret;

  if (!PyArg_ParseTuple(args, "Li", &duration, &num_rounds)) return NULL;
  ret = progressBy(duration, num_rounds);
  return Py_BuildValue("i", ret);
}

static PyObject *py_progressTimelineBy(PyObject *self, PyObject *args) {
  s64 duration;
  int timeline_id;
  int ret;

  if (!PyArg_ParseTuple(args, "iL", &timeline_id, &duration)) return NULL;
  ret = progressTimelineBy(timeline_id, duration);
  return Py_BuildValue("i", ret);
}

static PyObject *py_synchronizeAndFreeze(PyObject *self, PyObject *args) {
  int ret;
  ret = synchronizeAndFreeze();
  return Py_BuildValue("i", ret);
}

static PyMethodDef kronos_functions_methods[] = {
    {"synchronizeAndFreeze", py_synchronizeAndFreeze, METH_VARARGS, NULL},
    {"getTimePID", py_gettime_pid, METH_VARARGS, NULL},
    {"getCurrentVirtualTime", py_getCurrentVirtualTime, METH_VARARGS, NULL},
    {"setNetDeviceOwner", py_setNetDeviceOwner, METH_VARARGS, NULL},
    {"stopExp", py_stopExp, METH_VARARGS, NULL},
    {"initializeExp", py_initializeExp, METH_VARARGS, NULL},
    {"initializeVTExp", py_initializeVTExp, METH_VARARGS, NULL},
    {"progressBy", py_progress_by, METH_VARARGS, NULL},
    {"progressTimelineBy", py_progressTimelineBy, METH_VARARGS, NULL},
    {NULL, NULL, 0, NULL}};

#if PY_MAJOR_VERSION <= 2

void initkronos_functions(void) {
  Py_InitModule3("kronos_functions", kronos_functions_methods,
                 "Kronos API functions");
}

#elif PY_MAJOR_VERSION >= 3

static struct PyModuleDef kronos_api_definition = {
    PyModuleDef_HEAD_INIT, "kronos functions",
    "A Python module that exposes kronos module's API", -1, kronos_functions_methods};
PyMODINIT_FUNC PyInit_kronos_functions(void) {
  Py_Initialize();
  return PyModule_Create(&kronos_api_definition);
}

#endif
