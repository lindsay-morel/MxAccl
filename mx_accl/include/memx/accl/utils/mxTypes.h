#ifndef MXTYPES_H
#define MXTYPES_H
#include <vector>
#include <stdint.h>
#include <stdexcept>
#include <iostream>

// for MEMX_API_EXPORT macro
#include <memx/memx.h>

namespace MX
{
    namespace Types
    {

        class ShapeVector {
            private:
               std::vector<int64_t> shape;
               int64_t h=0, w=0, z=0, c=0;
               int size_ = 4;
            public:
                /**
                * @brief Construct a new ShapeVector type object
                */
                MEMX_API_EXPORT ShapeVector();// Initialize shape with 4 elements, all initialized to 0
                /**
                * @brief Construct a new ShapeVector type object
                * @param h Height
                * @param w Width
                * @param z Batch
                * @param c Channel
                */
                MEMX_API_EXPORT ShapeVector(int64_t h, int64_t w, int64_t z, int64_t c) ;
                /**
                * @brief Construct a new ShapeVector type object
                */
                MEMX_API_EXPORT ShapeVector(int size);// Initialize shape with size #elements, all initialized to 1
                // // Overload [] operator const
                // const int64_t& operator[](int64_t index) const ;
                // Overload [] operator
                MEMX_API_EXPORT int64_t& operator[](int64_t index) ;

                /**
                * @brief returns a vector of shape with channel first format
                */
                MEMX_API_EXPORT std::vector<int64_t> chfirst_shape();

                /**
                * @brief returns a vector of shape with channel last format
                */
                MEMX_API_EXPORT std::vector<int64_t> chlast_shape();

                /**
                * @brief returns a data pointer of the sape vector
                */
                MEMX_API_EXPORT int64_t* data();

                /**
                * @brief returns size of the shape vector
                */
                MEMX_API_EXPORT int64_t size() const ;

                /**
                 * @brief sets the shape to channel first format
                 */
                MEMX_API_EXPORT void set_ch_first();
        };

        //struct with necessary model information collated for internal and external purposes

        /** @struct MxModelInfo
            @brief struct with necessary information of a model
            @var MxModelInfo::model_index
            index of a model to identify
            @var MxModelInfo::num_in_featuremaps
            Number of input featuremaps
            @var MxModelInfo::num_out_featuremaps
            Number of output featuremaps
            @var MxModelInfo::input_layer_names
            Vector of strings containing input layer names
            @var MxModelInfo::output_layer_names
            Vector of strings containing output layer names
            @var MxModelInfo::in_featuremap_shapes
            Vector of Shapevector containing input featuremap shapes
            @var MxModelInfo::out_featuremap_shapes
            Vector of Shapevector containing output featuremap shapes
            @var MxModelInfo::in_featuremap_sizes
            Vector of size_t containing sizes fo the input featuremaps
            @var MxModelInfo::out_featuremap_sizes
            Vector of size_t containing sizes fo the output featuremaps
        */
        struct MxModelInfo{
            int model_index;
            int num_in_featuremaps;
            int num_out_featuremaps;
            std::vector<std::string> input_layer_names;
            std::vector<std::string> output_layer_names;
            std::vector<MX::Types::ShapeVector> in_featuremap_shapes;
            std::vector<MX::Types::ShapeVector> out_featuremap_shapes;
            std::vector<size_t> in_featuremap_sizes;
            std::vector<size_t> out_featuremap_sizes;
        };

        /**
         * @enum FrequencyOption
         * @brief Enumeration representing the available frequency options in MHz.
         *
         * Each frequency level corresponds to a specific computational performance in TFLOPS.
         */
        enum class MxFrequencyOption {
            FREQ_USE_CONF = 0,  ///< Use the current value of /etc/memryx/power.conf instead of overriding
            FREQ_200MHz = 200,  ///< 200 MHz provides 4.8 TFLOPS
            FREQ_300MHz = 300,  ///< 300 MHz provides 7.2 TFLOPS
            FREQ_400MHz = 400,  ///< 400 MHz provides 9.6 TFLOPS
            FREQ_450MHz = 450,  ///< 450 MHz provides 10.8 TFLOPS
            FREQ_500MHz = 500,  ///< 500 MHz provides 12.0 TFLOPS
            FREQ_600MHz = 600,  ///< 600 MHz provides 14.4 TFLOPS
            FREQ_700MHz = 700,  ///< 700 MHz provides 16.8 TFLOPS
            FREQ_750MHz = 750,  ///< 750 MHz provides 18.0 TFLOPS
            FREQ_800MHz = 800,  ///< 800 MHz provides 19.2 TFLOPS
            FREQ_850MHz = 850   ///< 850 MHz provides 20.0 TFLOPS
        };


        enum class MxVoltageOption {
            VOLT_680mV = 680,  ///< Voltage for 200 MHz and 300 MHz
            VOLT_690mV = 690,  ///< Voltage for 400 MHz
            VOLT_700mV = 700,  ///< Voltage for 450 MHz, 500 MHz, 600 MHz
            VOLT_750mV = 750,  ///< Voltage for 700 MHz
            VOLT_760mV = 760,  ///< Voltage for 750 MHz
            VOLT_780mV = 780   ///< Voltage for 800 MHz and 850 MHz
        };

        MEMX_API_EXPORT MxVoltageOption getVoltageFromFrequency(MxFrequencyOption freq);

    } // Namespace Types
} // Namespace MX


#endif
