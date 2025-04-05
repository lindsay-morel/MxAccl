#ifndef MX_ACCL
#define MX_ACCL

#include <string>
#include <stdint.h>
#include <atomic>
#include <thread>

#include <memx/accl/MxModel.h>
#include <memx/accl/dfp.h>
#include <memx/accl/utils/general.h>
#include <memx/accl/utils/featureMap.h>
#include <memx/accl/utils/path.h>
#include <memx/accl/DeviceManager.h>
#include <memx/accl/utils/mxTypes.h>

using namespace std;
struct daemon_items;

namespace MX
{
  namespace Runtime
  {
    class MxAccl
    {
    private:
      typedef std::function<bool(vector<const MX::Types::FeatureMap<uint8_t> *>, int stream_id)> int_callback_t;
      typedef std::function<bool(vector<const MX::Types::FeatureMap<float> *>, int stream_id)> float_callback_t;
    public:

      /**
       * @brief MxAccl constructor
       *
       * @param use_shared_mode This flag is 'false' by default, giving the MxAccl object direct control of the MXA. When set to 'true', MxAccl can be used in Shared mode which enables multiple processes on local (or remote) machines to share the MXA, with some potential performance penalty.
       * @param server_ip Server IP to connect to represented as string. Default IP address is 127.0.0.1, which is localhost.
       * @param server_port_base Starting port number as unsigned int, default is 10000. The server will use this port, **and** port+1 and port+2. For example, 10000, 10001, 10002.
       */
      MEMX_API_EXPORT MxAccl(bool use_shared_mode = false, std::string server_ip = "127.0.0.1", unsigned int server_port_base = 10000);

      /**
       * @brief Connect a dfp to MxAccl object. Currently only one connect_dfp per MxAccl object is allowed.
       *
       * @param file_path Absolute path of DFP file. char* and String types can also be passed.
       * @param device_ids_to_use IDs of MXA devices this process intends to use. takes in a vector of IDs and will return an error if an empty vector is passed
       *
       * @return dfp_id which is later to be passed in connect_stream function to specify that specific stream to a dfp
       */
      MEMX_API_EXPORT int connect_dfp(const std::filesystem::path dfp_path,std::vector<int>& device_ids_to_use);

      /**
       * @brief Connect a dfp to MxAccl object. Currently only one connect_dfp per MxAccl object is allowed.
       *
       * @param file_path Absolute path of DFP file. char* and String types can also be passed.
       * @param group_id GroupId of MPU this application is intended to use.
       * group_id is defaulted to 0, but needs to be provided if using
       * any other group
       *
       * @return dfp_id which is later to be passed in connect_stream function to specify that specific stream to a dfp
       */
      MEMX_API_EXPORT int connect_dfp(const std::filesystem::path dfp_path,int group_id = 0);

      /**
       * @brief Connect a dfp as bytes to MxAccl object. Currently only one connect_dfp per MxAccl object is allowed.
       *
       * @param dfp_bytes Raw uint8_t* pointer to DFP data
       * @param device_ids_to_use IDs of MXA devices this process intends to use. takes in a vector of IDs and will return an error if an empty vector is passed
       *
       * @return dfp_id which is later to be passed in connect_stream function to specify that specific stream to a dfp
       */
      MEMX_API_EXPORT int connect_dfp(const uint8_t *dfp_bytes, std::vector<int>& device_ids_to_use);

      /**
       * @brief Connect a dfp as bytes to MxAccl object. Currently only one connect_dfp per MxAccl object is allowed.
       *
       * @param dfp_bytes Raw uint8_t* pointer to DFP data
       * @param group_id GroupId of MPU this application is intended to use.
       * group_id is defaulted to 0, but needs to be provided if using
       * any other group
       *
       * @return dfp_id which is later to be passed in connect_stream function to specify that specific stream to a dfp
       */
      MEMX_API_EXPORT int connect_dfp(const uint8_t *dfp_bytes, int group_id = 0);

      //Destructor
      MEMX_API_EXPORT ~MxAccl();

      /**
       * @brief Start running inference.
       * All streams must be connected before calling this function
       */
      MEMX_API_EXPORT void start();

      /**
       * @brief Stop running inference.
       * Shouldn't be called before calling start.
       *
       */
      MEMX_API_EXPORT void stop();

      /**
       * @brief Wait for all the streams to be done streaming. This function waits
       * till all the started input callbacks have returned false.
       * Shouldn't be called before calling start.
       *
       */
      MEMX_API_EXPORT void wait();

      /**
       * @brief Get number of models in the compiled DFP
       *
       * @return Number of models
       */
      MEMX_API_EXPORT int get_num_models();

      /**
       * @brief Get number the number of streams connected to the object
       *
       * @return Number of streams
       */
      MEMX_API_EXPORT int get_num_streams();

      /**
       * @brief Get number of chips the dfp is compiled for
       *
       * @return Number of chips
       */
      MEMX_API_EXPORT int get_dfp_num_chips();

      /**
       * @brief Connect a stream to a model
       * - float_callback_t is a function pointer of type, bool foo(vector<const MX::Types::FeatureMap<float>*>, int).
       * - When this input callback function returns false, the corresponding stream is stopped and when all the streams stop,
       * wait() is executed.
       * - connect_stream should be called before calling start() or after calling stop().
       * @param in_cb -> input callback function used by this stream
       * @param out_cb -> output callback function used by this stream
       * @param stream_id -> Unique id given to this stream which can later
       *              be used in the corresponding callback functions
       * @param model_id -> Index of model this stream is intended to be connected
       * @param dfp_id -> id of dfp returned by connect_dfp() function
      */
      MEMX_API_EXPORT void connect_stream(float_callback_t in_cb, float_callback_t out_cb, int stream_id, int model_id=0, int dfp_id = 0);
      // /**
      //  * @brief Connect a stream to a model
      //  * - float_callback_t is a function pointer of type, bool foo(vector<const MX::Types::FeatureMap<float>*>, int).
      //  * - int_callback_t is a function pointer of type, bool foo(vector<const MX::Types::FeatureMap<int>*>, int).
      //  * - When this input callback function returns false, the corresponding stream is stopped and when all the streams stop,
      //  * wait() is executed.
      //  * - connect_stream should be called before calling start() or after calling stop().
      //  * @param in_cb -> input callback function used by this stream
      //  * @param out_cb -> output callback function used by this stream
      //  * @param stream_id -> Unique id given to this stream which can later
      //  *              be used in the corresponding callback functions
      //  * @param model_id -> Index of model this stream is intended to be connected
      // */
      // void connect_stream(int_callback_t in_cb, float_callback_t out_cb, int stream_id, int model_id=0);

      /**
       * @brief get information of a particular model such as number of in out featureMaps and in out layer names
       * @param model_id model ID or the index for the required information
       * @return if valid model_id then MxModelInfo model_info with necessary information else throw runtime error invalid model_id
      */
      MEMX_API_EXPORT MX::Types::MxModelInfo get_model_info(int model_id) const;

      /**
       * @brief get information of the pre-processing model set to a particular model such as number of in out featureMaps and their sizes and shapes
       * @param model_id model ID or the index for the required information
       * @return if valid model_id then MxModelInfo model_info with necessary information else throw runtime error invalid model_id
      */
      MEMX_API_EXPORT MX::Types::MxModelInfo get_pre_model_info(int model_id) const;

      /**
       * @brief get information of the post-processing model set to a particular model such as number of in out featureMaps and their sizes and shapes s
       * @param model_id model ID or the index for the required information
       * @return if valid model_id then MxModelInfo model_info with necessary information else throw runtime error invalid model_id
      */
      MEMX_API_EXPORT MX::Types::MxModelInfo get_post_model_info(int model_id) const;

      // User threading functions - No doxygen comments as we are releasing this for internal use
      /**
       * @brief Set the number of workers for input and output streams. The default is the number of streams
       * for both number of input and output streams as that provides the maximum performance. If this method is
       * not called before calling start(), the accl will run in default mode. This method should be called
       * after connecting all the required streams.
       *
       * @param input_num_workers Number of input workers
       * @param output_num_workers Number of output workers
       * @param model_idx Index of model to which the workers are intended to be assigned to. The default is set to 0
      */
      MEMX_API_EXPORT void set_num_workers(int input_num_workers, int output_num_workers,int model_idx=0);

      /**
       * @brief Connect the information of the post-processing model that has been cropped by the neural compiler
       *
       * @param post_model_path  Abosulte path of the post-processing model. (Can be onnx/tflite etc)
       * @param model_idx The index of model for which the post-processing is intended to be connected to.
       * @param post_size_list If the output of the post-processing has a variable size or if the ouput sizes
       * are not deduced, the maximum possible sizes of the output need to be passed. The default is an empty vector.
      */
      MEMX_API_EXPORT void connect_post_model(std::filesystem::path post_model_path, int model_idx=0, const std::vector<size_t>& post_size_list={});

      /**
       * @brief Connect the information of the pre-processing model that has been cropped by the neural compiler
       *
       * @param pre_model_path  Abosulte path of the pre-processing model. (Can be onnx/tflite etc)
       * @param model_idx The index of model for which the post-processing is intended to be connected to.
      */
      MEMX_API_EXPORT void connect_pre_model(std::filesystem::path pre_model_path, int model_idx=0);

      /**
       * @brief Configure multi-threaded FeatureMap data conversion using the given number of threads.
       * Conversion multithreading is mainly intended for high FPS single-stream scenarios, or userThreading mode.
       * In multi-stream autoThreading scenarios, this option should not be necessary, and may even
       * degrade performance due to increased CPU load.
       *
       * @param num_threads Number of worker threads for FeatureMaps. Use >= 2 to enable. Values < 2 disable.
       * @param model_idx Index of model to enable the feature  The default is set to 0
      */
      MEMX_API_EXPORT void set_parallel_fmap_convert(int num_threads, int model_idx=0);

      /**
         * @brief Checks if power consumption data can be retrieved for the connected modules.
         *
         * @return true if power consumption data is available, false otherwise.
       */
      MEMX_API_EXPORT bool can_get_power_consumption();

      /**
         * @brief Retrieves the average power consumption of all connected devices.
         *
         * @return A reference to a vector containing the average power consumption values (in watts) for all devices.
       */
      MEMX_API_EXPORT const std::vector<float>& get_avg_power_all_devices();

      /**
         * @brief Retrieves the current maximum temperature for all connected devices.
         *
         * @return A reference to a vector containing the max temperature values (in degrees Celsius) for all devices.
       */
      MEMX_API_EXPORT const std::vector<float>& get_max_temperature_all_devices();

      /**
         * @brief Retrieves temperatures of all chips across all open devices
         *
         * @return A reference to a vector<vector> containing the average temperature values (in degrees Celsius) for all devices.
       */
      MEMX_API_EXPORT const std::vector<std::vector<uint64_t>>& get_chip_temperatures_all_devices();


      /**
         * @brief Sets the operating frequency of the device.
         *
         * @note This function must be called before invoking `connect_dfp()`.
         *       Calling it after `connect_dfp()` has will throw runtim error.
         *
         * @param freq_option The desired frequency option. Defaults to 600 MHz if not specified.
         * @return true if the frequency was successfully set, false otherwise.
       */
      MEMX_API_EXPORT bool set_operating_frequency(MX::Types::MxFrequencyOption freq_option = MX::Types::MxFrequencyOption::FREQ_600MHz);


    private:
      std::filesystem::path dfp_path;
      std::vector<std::filesystem::path> dfp_paths;
      bool dfp_valid;
      bool setup_status;

      int group; //Group of the chip connected

      int dfp_tag;

      std::atomic_bool run;//Flag to know status of the Accl

      std::vector<ModelBase *> models;//Vector of all model objects

      MX::Runtime::DeviceManager *device_manager;

      std::unique_ptr<daemon_items> daemon_items_;
      Dfp::DfpObject* dfp_=NULL;
      void init_mx_models(std::vector<int>& device_ids_to_use);
      int num_models_;
      int num_chips_;
      std::vector<int> device_ids_;
      std::vector<int> context_ids_vector_;
      char uuid_str[37];
      std::vector<std::string> models_uuid;
      std::thread* heartbeat_thread;
      std::thread* local_heartbeat_thread;
      void heartbeat_fun();
      void local_heartbeat_fun();
      std::atomic_bool heartbeat_run;
      std::atomic_bool local_heartbeat_run;

    };
  } // namespace Runtime
} // namespace MX

#endif
