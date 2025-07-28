// Copyright (c) 2025 MemryX
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef MX_TYPES_H
#define MX_TYPES_H

#pragma once
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

/**
 * @class ShapeVector
 * @brief Represents the shape of a tensor with flexible dimension ordering.
 *
 * The ShapeVector class encapsulates tensor shape information using a fixed-size or dynamic vector.
 * It provides convenience methods for interpreting and converting between channel-first and 
 * channel-last formats. Internally, the shape is represented using four components by default:
 * height (h), width (w), batch (z), and channel (c), but custom-sized shapes are also supported.
 */
class ShapeVector
{
private:
    std::vector<int64_t> shape; ///< Underlying storage for shape dimensions.
    int64_t h = 0, w = 0, z = 0, c = 0; ///< Canonical height, width, batch, and channel dimensions.
    int size_ = 4; ///< Default shape size.

public:
    /**
     * @brief Default constructor. Initializes the shape with 4 dimensions, all set to 0.
     */
    MEMX_API_EXPORT ShapeVector();

    /**
     * @brief Construct a ShapeVector using explicit dimensions.
     * @param h Height.
     * @param w Width.
     * @param z Batch size.
     * @param c Channel count.
     */
    MEMX_API_EXPORT ShapeVector(int64_t h, int64_t w, int64_t z, int64_t c);

    /**
     * @brief Construct a ShapeVector of custom size with all dimensions initialized to 1.
     * @param size Number of dimensions.
     */
    MEMX_API_EXPORT ShapeVector(int size);

    /**
     * @brief Accessor for a specific dimension by index (non-const).
     * @param index Dimension index.
     * @return Reference to the dimension value at the given index.
     */
    MEMX_API_EXPORT int64_t& operator[](int64_t index);

    /**
     * @brief Returns the shape in channel-first format (e.g., [C, H, W, Z]).
     * @return A vector of dimensions in channel-first order.
     */
    MEMX_API_EXPORT std::vector<int64_t> chfirst_shape();

    /**
     * @brief Returns the shape in channel-last format (e.g., [H, W, Z, C]).
     * @return A vector of dimensions in channel-last order.
     */
    MEMX_API_EXPORT std::vector<int64_t> chlast_shape();

    /**
     * @brief Returns a pointer to the raw shape data.
     * @return Pointer to the first element of the shape vector.
     */
    MEMX_API_EXPORT int64_t* data();

    /**
     * @brief Returns the number of dimensions in the shape.
     * @return Number of dimensions.
     */
    MEMX_API_EXPORT int64_t size() const;

    /**
     * @brief Sets the internal shape to follow channel-first layout.
     */
    MEMX_API_EXPORT void set_ch_first();
};

/**
 * @struct MxModelInfo
 * @brief Holds metadata and configuration details for a compiled model.
 *
 * The MxModelInfo struct contains essential information about a model compiled
 * for execution, including the number of input and output feature maps, their 
 * shapes and sizes, and the associated layer names. This metadata is typically 
 * used during runtime setup, validation, or for constructing input/output buffers.
 *
 * @var MxModelInfo::model_index
 * Unique index identifying the model instance.

 * @var MxModelInfo::num_in_featuremaps
 * Number of input feature maps required by the model.

 * @var MxModelInfo::num_out_featuremaps
 * Number of output feature maps produced by the model.

 * @var MxModelInfo::input_layer_names
 * Names of the model's input layers, listed in the order expected by the runtime.

 * @var MxModelInfo::output_layer_names
 * Names of the model's output layers, listed in the order produced by the model.

 * @var MxModelInfo::in_featuremap_shapes
 * Shapes of the input feature maps, represented as a vector of ShapeVector objects.

 * @var MxModelInfo::out_featuremap_shapes
 * Shapes of the output feature maps, represented as a vector of ShapeVector objects.

 * @var MxModelInfo::in_featuremap_sizes
 * Sizes (in bytes or elements, depending on context) of each input feature map.

 * @var MxModelInfo::out_featuremap_sizes
 * Sizes (in bytes or elements, depending on context) of each output feature map.
 */
struct MxModelInfo {
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
enum MxFrequencyOption : uint16_t {
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


enum MxVoltageOption : uint16_t {
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
