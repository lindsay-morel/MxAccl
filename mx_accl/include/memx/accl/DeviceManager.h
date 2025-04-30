#ifndef DEVICE_MANAGER
#define DEVICE_MANAGER

#include <string>
#include <stdint.h>
#include <atomic>
#include <iostream>
#include <unordered_map>
#include <memx/memx.h>
#include <memx/accl/dfp.h>
#include <memx/accl/MxModel.h>
#include <memx/accl/utils/path.h>
#include <memx/accl/utils/mxTypes.h>

using mx_retval_t = MX::Utils::mx_retval;

namespace MX
{
  namespace Runtime
  {
    struct dfp_rt_info{
      std::filesystem::path dfp_filename_path;
      Dfp::DfpObject* dfp;
      int dfp_num_chips;//Number of chips DFP is compiled
      float mxa_gen;//Generation of chip DFP is compiled
      int num_models;//Number of models DFP is compiled
      Dfp::DfpMeta dfp_meta;
      std::vector<int> context_ids_vector;
      bool valid;
      bool is_bytes;
      bool use_multigroup_lb;
    };

    struct device_info{
      int chip_count;
      bool is_device_open;
      int number_of_contexts_attached; // should be less than 32 per device
      std::vector<int> contexts_ids_attached;
      int current_config;
      // int last_context_attached;
    };


    class DeviceManager{
      public:
        MEMX_API_EXPORT DeviceManager(void* stub = NULL, bool server_mode = false);
        MEMX_API_EXPORT mx_retval_t opendfp(const std::filesystem::path dfp_filename, int dfp_tag);
        MEMX_API_EXPORT mx_retval_t opendfp_bytes(const uint8_t *b, int dfp_tag);
        MEMX_API_EXPORT mx_retval_t setup_mxa(int dfp_tag, std::vector<int>& pgroup_ids);
        MEMX_API_EXPORT mx_retval_t attach_dfp_to_device(int dfp_tag);
        MEMX_API_EXPORT mx_retval_t download_dfp_to_device(int dfp_tag);
        MEMX_API_EXPORT mx_retval_t init_mx_models(int dfp_tag, std::vector<ModelBase *>* mxmodel_vector );

        //Getter functions
        MEMX_API_EXPORT int get_dfp_num_chips(int dfp_tag);
        MEMX_API_EXPORT int get_dfp_num_models(int dfp_tag);
        MEMX_API_EXPORT bool get_dfp_validity(int dfp_tag);

        MEMX_API_EXPORT mx_retval_t close_all_devices();
        MEMX_API_EXPORT void cleanup__all_dfps();
        // void cleanup_all_setup_maps();
        // static void update_context_tracker_id();
        // static int get_context_tracker_id();

        // bool dfp_tag_duplicate_check(int dfp_tag);
        MEMX_API_EXPORT void print_available_devices();
        MEMX_API_EXPORT void cleanup_dfp(int dfp_tag);
        MEMX_API_EXPORT mx_retval_t close_device(int device_id);
        MEMX_API_EXPORT int get_num_outports(int dfp_tag);
        bool power_data_possible_or_no();
        const std::vector<float>& get_power_all_open_devices();
        const std::vector<float>& get_max_temperature_all_open_devices();
        const std::vector<std::vector<uint64_t>>& get_chip_temperature_all_open_devices();

        void set_frequency(uint16_t freq);
        void set_volt(uint16_t volt);
              // void isflash_module();


        /*
        // Additional get function disabled for now but might need later
        float get_dfp_mxa_gen();
        int get_connected_devices_count();
        int get_available_device_count();
        Dfp::DfpMeta get_dfp_meta();
        */

      private:

        mx_retval_t get_available_devices();
        mx_retval_t throw_chip_exception(int pdfp_chips, int pdevice_chips, int device_id);
        mx_retval_t throw_mxa_gen_exception(int pdfp_num_chips);
        mx_retval_t configure_device(int device_id, int device_chip_count, int pdfp_num_chips, float pmx_gen);
        mx_retval_t throw_device_not_available_exception(int pdevice_id);
        mx_retval_t connect_device(int dfp_tag, int device_id);
        void identify_flash_modules();

        void read_power_mode();
        void set_power_mode(int device_id, int num_chips);


        std::vector<bool> isflashmodule_vec;
        bool can_return_power_data = false;

        using mxmaptype = std::unordered_map<int, MX::Runtime::dfp_rt_info>;
        mxmaptype dfp_mxa_map;

        using mxa_device_map_type = std::unordered_map< int, MX::Runtime::device_info>;
        mxa_device_map_type available_mxa_device_map;

        int all_devices_count;
        int required_devices;
        int number_of_context_per_dfp;
        int group_id_passed;
        std::vector<int> available_devices_id;
        std::vector<int> open_devices;

        void* stub_;
        mx_retval_t try_lock(int grp_id);
        bool server_mode_;
        mx_retval_t device_unlock(int device_id);
        std::vector<float> device_powers;
        std::vector<float> device_max_temperatures;
        std::vector<std::vector<uint64_t>> chip_temperatures;

        // Frequency members
        uint16_t c4_freq = 600;
        uint16_t c4_volt = 700;
        uint16_t c2_freq = 600;
        uint16_t c2_volt = 700;

    };// DeviceManager

 } // namespace Runtime
} // namespace MX

#endif
