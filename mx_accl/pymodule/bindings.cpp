#include <pybind11/pybind11.h>
#include <pybind11/stl.h>      // for std::string
#include <pybind11/stl/filesystem.h> // for std::filesystem::path
#include <pybind11/pytypes.h>  // for py::bytes
#include <pybind11/numpy.h>

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>
#include <numpy/ndarrayobject.h>
#include <numpy/ndarraytypes.h>

#include <memx/accl/MxAccl.h>
#include <memx/accl/dfp.h>
#include <memx/accl/messages.h>
#include <memx/accl/utils/macros.h>

#include <memx/memx.h>

#include <string>

// has all the gbf convert stuff
#include "convert.h"

namespace py = pybind11;

using namespace MX::Runtime;
using namespace MX::RPC;

/***************************************************************************
 * This script may seem weird because it combines the Python C API with pybind11.
 * Since the driver mxa module already uses the Python C API, we reuse that code here to 
 * avoid reinventing the wheel. And pybind11 is used on top of it to provide a C++ 
 * wrapper interface.
 ******************************************************************************/


//-------------------------------------------------------------------------------
// Python C API
//-------------------------------------------------------------------------------
static PyObject* stream_ifmap(PyObject* self, PyObject* args, PyObject *kwargs)
{
  bool status;
  uint8_t flow_id;
  PyArrayObject* ifmap;
  PyObject* Py_client;

  uint16_t height = 0;
  uint16_t width = 0;
  uint16_t z = 0;
  uint32_t num_ch = 0;
  uint32_t tensor_size = 0;
  uint64_t fmt_size = 0;
  uint8_t format = 0;
  PyArrayObject* buffer;

  // parse parameters
  static char *kwlist[] = {"flow_id", "ifmap", "client", "fmt_size", "tensor_size", "format", "dim_h", "dim_w", "dim_z", "dim_c", "buffer", nullptr};
  if(!PyArg_ParseTupleAndKeywords(args, kwargs, "bO!OKIbHHHIO!", kwlist, &flow_id, &PyArray_Type, &ifmap, &Py_client, &fmt_size, &tensor_size, &format, &height, &width, &z, &num_ch, &PyArray_Type, &buffer)) {
    PyErr_BadArgument();
    return nullptr;
  }

  // convert to client 
  py::handle handle_client(Py_client);
  MX::Runtime::Client* client = handle_client.cast<MX::Runtime::Client*>();

  uint8_t *formatted_data = (uint8_t*)PyArray_DATA(buffer);

  {
      // printf("input shape info => height: %d, width: %d, z: %d, num_ch: %d, format: %d, tensor_size: %d\n", height, width, z, num_ch, format, tensor_size);

      if(format == MEMX_FMAP_FORMAT_BF16){
          // BF convert
          Py_INCREF(ifmap);
          Py_INCREF(buffer);
          Py_BEGIN_ALLOW_THREADS
          
          convert_bf16( (uint32_t*)PyArray_DATA(ifmap), formatted_data, tensor_size );

          // send
          status = client->send(formatted_data, fmt_size);
          // printf("[client %d] send ifmap data size: %d\n", client->my_client_id, fmt_size);

          Py_END_ALLOW_THREADS
          Py_DECREF(buffer);
          Py_DECREF(ifmap);
      } else if (format == MEMX_FMAP_FORMAT_GBF80) {
          Py_INCREF(ifmap);
          Py_INCREF(buffer);
          Py_BEGIN_ALLOW_THREADS
         
          convert_gbf( (uint32_t*)PyArray_DATA(ifmap), formatted_data, tensor_size, num_ch );

          // send
          status = client->send(formatted_data, fmt_size);
          // printf("[client %d] send ifmap data size: %d\n", client->my_client_id, fmt_size);

          Py_END_ALLOW_THREADS
          Py_DECREF(buffer);
          Py_DECREF(ifmap);
      } else if (format == MEMX_FMAP_FORMAT_GBF80_ROW_PAD) {
          // GBF row pad
          Py_INCREF(ifmap);
          Py_INCREF(buffer);
          Py_BEGIN_ALLOW_THREADS
          
          convert_gbf_row_pad( (uint32_t*)PyArray_DATA(ifmap), formatted_data, height, width, z, num_ch);

          // send
          status = client->send(formatted_data, fmt_size);
          // printf("[client %d] send ifmap data size: %d\n", client->my_client_id, fmt_size);

          Py_END_ALLOW_THREADS
          Py_DECREF(buffer);
          Py_DECREF(ifmap);
      } else {
          // don't convert anything else
          Py_INCREF(ifmap);
          Py_BEGIN_ALLOW_THREADS
          status = client->send( (uint8_t*)PyArray_DATA(ifmap), fmt_size);
          // printf("[client %d] send ifmap data size: %d\n", client->my_client_id, fmt_size);
          Py_END_ALLOW_THREADS
          Py_DECREF(ifmap);
      }
  }

  unused(self);
  return Py_BuildValue("i", status);
}

static PyObject* stream_ofmap(PyObject* self, PyObject* args, PyObject *kwargs)
{
  bool status;
  uint8_t flow_id;
  PyArrayObject* ofmap;
  PyObject* Py_client;
  
  uint16_t height = 0;
  uint16_t width = 0;
  uint16_t z = 0;
  uint32_t num_ch = 0;
  uint32_t tensor_size = 0;
  uint64_t fmt_size = 0;
  uint8_t format = 0;
  uint8_t hpoc_enabled = 0;
  int hpoc_size = 0;
  PyArrayObject* hpoc_indexes = nullptr;
  PyArrayObject* buffer = nullptr;

  // parse parameters
  static char *kwlist[] = {"flow_id", "ofmap", "client", "fmt_size", "tensor_size", "format", "dim_h", "dim_w", "dim_z", "dim_c", "hpoc_enabled", "hpoc_extra_size", "hpoc_indexes", "buffer", nullptr};
  if(!PyArg_ParseTupleAndKeywords(args, kwargs, "bO!OKIbHHHIbIO!O!", kwlist, &flow_id, &PyArray_Type, &ofmap, &Py_client, &fmt_size, &tensor_size, &format, &height, &width, &z, &num_ch, &hpoc_enabled, &hpoc_size, &PyArray_Type, &hpoc_indexes, &PyArray_Type, &buffer)) {
    PyErr_BadArgument();
    return nullptr;
  }

  // convert to client 
  py::handle handle_client(Py_client);
  MX::Runtime::Client* client = handle_client.cast<MX::Runtime::Client*>();

  uint8_t *formatted_data = (uint8_t*)PyArray_DATA(buffer);

  {
    // printf("output shape info => height: %d, width: %d, z: %d, num_ch: %d, format: %d, tensor_size: %d\n", height, width, z, num_ch, format, tensor_size);
    
    if(format == MEMX_FMAP_FORMAT_BF16){
        Py_INCREF(ofmap);
        Py_INCREF(buffer);
        Py_BEGIN_ALLOW_THREADS
        
        // printf("[client %d] About to receive ofmap data size: %d\n", client->my_client_id, fmt_size);
        status = client->recv(formatted_data, fmt_size);
        // printf("[client %d] Finish receiving ofmap\n", client->my_client_id);
        
        // BF unconvert
        unconvert_bf16(formatted_data, (uint32_t*)PyArray_DATA(ofmap), tensor_size);

        Py_END_ALLOW_THREADS
        Py_DECREF(buffer);
        Py_DECREF(ofmap);

    } else if(format == MEMX_FMAP_FORMAT_GBF80){
        Py_INCREF(ofmap);
        Py_INCREF(buffer);
        Py_INCREF(hpoc_indexes);
        Py_BEGIN_ALLOW_THREADS

        // printf("[client %d] About to receive ofmap data size: %d\n", client->my_client_id, fmt_size);
        status = client->recv(formatted_data, fmt_size);
        // printf("[client %d] Finish receiving ofmap\n", client->my_client_id);

        // GBF unconvert
        if (hpoc_enabled != 0) {
          unconvert_gbf_hpoc(formatted_data, (uint32_t*)PyArray_DATA(ofmap), height, width, z, num_ch, hpoc_size, (int*)PyArray_DATA(hpoc_indexes), 0);
        } else {
          unconvert_gbf(formatted_data, (uint32_t*)PyArray_DATA(ofmap), tensor_size, num_ch);
        }

        Py_END_ALLOW_THREADS
        Py_DECREF(ofmap);
        Py_DECREF(buffer);
        Py_DECREF(hpoc_indexes);

    } else if(format == MEMX_FMAP_FORMAT_GBF80_ROW_PAD){
        Py_INCREF(ofmap);
        Py_INCREF(buffer);
        Py_INCREF(hpoc_indexes);
        Py_BEGIN_ALLOW_THREADS

        // printf("[client %d] About to receive ofmap data size: %d\n", client->my_client_id, fmt_size);
        status = client->recv(formatted_data, fmt_size);
        // printf("[client %d] Finish receiving ofmap\n", client->my_client_id);

        // GBF unconvert
        if (hpoc_enabled != 0) {
          unconvert_gbf_hpoc(formatted_data, (uint32_t*)PyArray_DATA(ofmap), height, width, z, num_ch, hpoc_size, (int*)PyArray_DATA(hpoc_indexes), 1);
        } else {
          unconvert_gbf_row_pad(formatted_data, (uint32_t*)PyArray_DATA(ofmap), height, width, z, num_ch);
        }

        Py_END_ALLOW_THREADS
        Py_DECREF(ofmap);
        Py_DECREF(buffer);
        Py_DECREF(hpoc_indexes);

    } else {
        // don't convert anything else
        Py_INCREF(ofmap);
        Py_BEGIN_ALLOW_THREADS
        // printf("[client %d] About to receive ofmap data size: %d\n", client->my_client_id, fmt_size);
        status = client->recv((uint8_t*)PyArray_DATA(ofmap), fmt_size);
        // printf("[client %d] Finish receiving ofmap\n", client->my_client_id);
        Py_END_ALLOW_THREADS
        Py_DECREF(ofmap);
    
    }
  }

  unused(self);
  return Py_BuildValue("i", status);
}

// Forward declaration for registering raw C API functions
static PyMethodDef py_methods[] = {
    {"stream_ifmap", (PyCFunction)stream_ifmap, METH_VARARGS | METH_KEYWORDS, "stream_ifmap in shared mode"},
    {"stream_ofmap", (PyCFunction)stream_ofmap, METH_VARARGS | METH_KEYWORDS, "stream_ofmap in shared mode"},
    {nullptr, nullptr, 0, nullptr}
};

//-------------------------------------------------------------------------------
// PYBIND11_MODULE
//-------------------------------------------------------------------------------

// helper to safely call import_array()
static int numpy_import_array_wrapper() {
    import_array();  // init. numpy array is required in the very beginning
    return 0;
}

PYBIND11_MODULE(mxapi, m) {

    numpy_import_array_wrapper(); // init. numpy array is required in the very beginning

    // Register raw C API functions
    PyObject* mod_ptr = m.ptr();
    PyModule_AddFunctions(mod_ptr, py_methods);

    // SchedulerOptions
    py::class_<MX::RPC::SchedulerOptions>(m, "SchedulerOptions")
        .def(py::init<int, int, bool, int, int>())
        .def_readwrite("frame_limit", &MX::RPC::SchedulerOptions::frame_limit)
        .def_readwrite("time_limit", &MX::RPC::SchedulerOptions::time_limit)
        .def_readwrite("stop_on_empty", &MX::RPC::SchedulerOptions::stop_on_empty)
        .def_readwrite("ifmap_queue_size", &MX::RPC::SchedulerOptions::ifmap_queue_size)
        .def_readwrite("ofmap_queue_size", &MX::RPC::SchedulerOptions::ofmap_queue_size);
    
    // ClientOptions
    py::class_<MX::RPC::ClientOptions>(m, "ClientOptions")
        .def(py::init<bool, float>())
        .def_readwrite("smoothing", &MX::RPC::ClientOptions::smoothing)
        .def_readwrite("fps_target", &MX::RPC::ClientOptions::fps_target);

    // DfpObject
    py::class_<Dfp::DfpObject>(m, "DfpObject")
        .def(py::init<std::string>(), py::arg("filename"))
        .def_readonly("dfp_byte_size", &Dfp::DfpObject::dfp_byte_size)
        .def("get_src_dfp_bytes", [](Dfp::DfpObject& self, size_t length) {
                return py::bytes(reinterpret_cast<const char*>(self.src_dfp_bytes), length);
            }, 
            py::arg("length"), "Returns DFP bytes as Python bytes")
        .def_property_readonly("dfp_version_str", [](Dfp::DfpObject& self) {
            return self.get_dfp_meta()->dfp_version_str;
        })
        .def_property_readonly("dfp_version", [](Dfp::DfpObject& self) {
            return self.get_dfp_meta()->dfp_version;
        })
        .def_property_readonly("compile_time", [](Dfp::DfpObject& self) {
            return self.get_dfp_meta()->compile_time;
        })
        .def_property_readonly("compiler_version", [](Dfp::DfpObject& self) {
            return self.get_dfp_meta()->compiler_version;
        })
        .def_property_readonly("chip_gen", [](Dfp::DfpObject& self) {
            return self.get_dfp_meta()->mxa_gen;
        })
        .def_property_readonly("chip_gen_name", [](Dfp::DfpObject& self) {
            return self.get_dfp_meta()->mxa_gen_name;
        })
        .def_property_readonly("num_chips", [](Dfp::DfpObject& self) {
            return self.get_dfp_meta()->num_chips;
        })
        .def_property_readonly("use_multigroup_lb", [](Dfp::DfpObject& self) {
            return self.get_dfp_meta()->use_multigroup_lb;
        })
        .def_property_readonly("num_inports", [](Dfp::DfpObject& self) {
            return self.get_dfp_meta()->num_inports;
        })
        .def_property_readonly("num_outports", [](Dfp::DfpObject& self) {
            return self.get_dfp_meta()->num_outports;
        })
        .def_property_readonly("num_used_inports", [](Dfp::DfpObject& self) {
            return self.get_dfp_meta()->num_used_inports;
        })
        .def_property_readonly("num_used_outports", [](Dfp::DfpObject& self) {
            return self.get_dfp_meta()->num_used_outports;
        })
        .def_property_readonly("num_models", [](Dfp::DfpObject& self) {
            return self.get_dfp_meta()->num_models;
        })
        .def_property_readonly("model_inports", [](Dfp::DfpObject& self) {
            return self.get_dfp_meta()->model_inports;
        })
        .def_property_readonly("model_outports", [](Dfp::DfpObject& self) {
            return self.get_dfp_meta()->model_outports;
        });

    //  Device Info device_info_t
    py::class_<MX::RPC::device_info_t>(m, "device_info_t")
        .def(py::init<int32_t, int32_t, int32_t, int32_t, bool, bool, std::vector<uint16_t>, uint16_t>())
        .def_readwrite("chip_count", &MX::RPC::device_info_t::chip_count)
        .def_readwrite("current_config", &MX::RPC::device_info_t::current_config)
        .def_readwrite("num_groups", &MX::RPC::device_info_t::num_groups)
        .def_readwrite("chips_per_group", &MX::RPC::device_info_t::chips_per_group)
        .def_readwrite("can_get_power_data", &MX::RPC::device_info_t::can_get_power_data)
        .def_readwrite("is_usb", &MX::RPC::device_info_t::is_usb)
        .def_readwrite("freqs", &MX::RPC::device_info_t::freqs)
        .def_readwrite("volt", &MX::RPC::device_info_t::volt)
        .def("__getitem__", [](const MX::RPC::device_info_t &self, const std::string &key) -> py::object {
            if (key == "chip_count") return py::cast(self.chip_count);
            if (key == "current_config") return py::cast(self.current_config);
            if (key == "num_groups") return py::cast(self.num_groups);
            if (key == "chips_per_group") return py::cast(self.chips_per_group);
            if (key == "can_get_power_data") return py::cast(self.can_get_power_data);
            if (key == "freqs") return py::cast(self.freqs);
            if (key == "volt") return py::cast(self.volt);
            throw std::out_of_range("Invalid key in device_info_t: " + key);
        })
        .def("__contains__", [](const MX::RPC::device_info_t &, const std::string &key) {
            return key == "chip_count" || key == "current_config" || key == "num_groups" ||
                key == "chips_per_group" || key == "can_get_power_data" ||
                key == "freqs" || key == "volt";
        })
        .def("keys", []() {
            return std::vector<std::string>{
                "chip_count", "current_config", "num_groups",
                "chips_per_group", "can_get_power_data", "freqs", "volt"
            };
        });



    // Client 
    py::class_<MX::Runtime::Client>(m, "Client")
        .def(py::init<>())
        .def("init_connection", &MX::Runtime::Client::init_connection,
            py::arg("server_address") = "/run/mxa_manager/",
            py::arg("base_port") = 10000)
        .def("end_connection", &MX::Runtime::Client::end_connection)
        .def("get_avg_max_temp", &MX::Runtime::Client::get_avg_max_temp,
            py::arg("device_id"))
        .def("get_inst_max_temp", &MX::Runtime::Client::get_inst_max_temp,
            py::arg("device_id"))
        .def("get_avg_power", &MX::Runtime::Client::get_avg_power,
            py::arg("device_id"))
        .def("get_inst_power", &MX::Runtime::Client::get_inst_power,
            py::arg("device_id"))
        .def("set_power_mode", &MX::Runtime::Client::set_power_mode,
            py::arg("device_id"),
            py::arg("freq_mhz"))
        .def("get_pressure", [](
                // wrapper to return pressure as string "low", "medium", "high", "full"
                // instead of a float number
                MX::Runtime::Client& self,
                int32_t device_id) {
                    float pressure = self.get_pressure(device_id);
                    if (pressure < MEMX_PRESSURE_LOW_THRESH) {
                        return std::string("low");
                    } else if (pressure < MEMX_PRESSURE_MEDIUM_THRESH) {
                        return std::string("medium");
                    } else if (pressure < MEMX_PRESSURE_HIGH_THRESH) {
                        return std::string("high");
                    } else {
                        return std::string("full");
                    }
            },
            py::arg("device_id"))
        .def("try_local_lock", &MX::Runtime::Client::try_local_lock,
            py::arg("device_id"))
        .def("local_unlock", &MX::Runtime::Client::local_unlock,
            py::arg("device_id"))
        .def("get_device_infos", &MX::Runtime::Client::get_device_infos)
        .def("connect_dfp", [](
                MX::Runtime::Client& self,
                py::bytes dfp_bytes,
                int32_t model_id,
                const SchedulerOptions& sched_options,
                const ClientOptions& client_options,
                std::vector<int32_t> devices_to_use) {
                    auto info = py::buffer(dfp_bytes).request();
                    return self.connect_dfp(
                            info.size,
                            static_cast<uint8_t*>(info.ptr),
                            model_id,
                            sched_options,
                            client_options,
                            static_cast<int32_t>(devices_to_use.size()),
                            devices_to_use.data());
            }, 
            py::arg("dfp_bytes"),
            py::arg("model_id"),
            py::arg("scheduler_options"),
            py::arg("client_options"),
            py::arg("devices_to_use"))
        .def_readwrite("my_client_id", &MX::Runtime::Client::my_client_id);

    // expose the MEMX_PRESSURE_* constants
    m.attr("MEMX_PRESSURE_LOW_THRESH") = MEMX_PRESSURE_LOW_THRESH;
    m.attr("MEMX_PRESSURE_MEDIUM_THRESH") = MEMX_PRESSURE_MEDIUM_THRESH;
    m.attr("MEMX_PRESSURE_HIGH_THRESH") = MEMX_PRESSURE_HIGH_THRESH;
}
