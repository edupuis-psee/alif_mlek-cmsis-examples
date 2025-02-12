/*
 * SPDX-FileCopyrightText: Copyright 2021-2023 Arm Limited and/or its
 * affiliates <open-source-office@arm.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * This object detection example is intended to work with the
 * CMSIS pack produced by ml-embedded-eval-kit. The pack consists
 * of platform agnostic end-to-end ML use case API's that can be
 * used to construct ML examples for any target that can support
 * the memory requirements for TensorFlow-Lite-Micro framework and
 * some heap for the API runtime.
 */
#include "BufAttributes.hpp" /* Buffer attributes to be applied */
#include <random>
#include "TestModel.hpp"

/* Platform dependent files */
#include "RTE_Components.h"  /* Provides definition for CMSIS_device_header */
#include CMSIS_device_header /* Gives us IRQ num, base addresses. */
#include "BoardInit.hpp"      /* Board initialisation */
#include "log_macros.h"      /* Logging macros (optional) */
#include "board.h"
#include "app_map.h"
#include "global_map.h"
#include "Driver_GPIO.h"
#include "pinconf.h"

namespace arm {
namespace app {
    /* Tensor arena buffer */
    static uint8_t tensorArena[ACTIVATION_BUF_SZ] ACTIVATION_BUF_ATTRIBUTE;

    /* Optional getter function for the model pointer and its size. */
    namespace object_detection {
        extern uint8_t* GetModelPointer();
        extern size_t GetModelLen();
    } /* namespace object_detection */
} /* namespace app */
} /* namespace arm */

#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
__asm("  .global __ARM_use_no_argv\n");
#endif

#define _GET_DRIVER_REF(ref, peri, chan) \
    extern ARM_DRIVER_##peri Driver_##peri##chan; \
    static ARM_DRIVER_##peri * ref = &Driver_##peri##chan;
#define GET_DRIVER_REF(ref, peri, chan) _GET_DRIVER_REF(ref, peri, chan)

// Prepare GPIO5 driver
GET_DRIVER_REF(BOARD_GPIO_5_DRV, GPIO, PORT_5);


/**
 * @brief Initializes the GPIO pins.
 *
 * This function sets up the necessary GPIO pins for the application.
 * It configures the pins as input or output as required.
 */
void initialize_gpio_pins() {
    uint32_t config_gpio =
        PADCTRL_READ_ENABLE |
        PADCTRL_SCHMITT_TRIGGER_ENABLE |
        PADCTRL_OUTPUT_DRIVE_STRENGTH_4MA;

    // GPIO5 pin 6 constant high for logic analyzer ref
    BOARD_GPIO_5_DRV->Initialize(PIN_6, NULL);
    BOARD_GPIO_5_DRV->PowerControl(PIN_6, ARM_POWER_FULL);
    BOARD_GPIO_5_DRV->SetDirection(PIN_6, GPIO_PIN_DIRECTION_OUTPUT);
    BOARD_GPIO_5_DRV->SetValue(PIN_6, GPIO_PIN_OUTPUT_STATE_HIGH);
    pinconf_set(PORT_5, PIN_6, PINMUX_ALTERNATE_FUNCTION_0, config_gpio);

    // GPIO5 pin 4 high during UUT
    BOARD_GPIO_5_DRV->Initialize(PIN_4, NULL);
    BOARD_GPIO_5_DRV->PowerControl(PIN_4, ARM_POWER_FULL);
    BOARD_GPIO_5_DRV->SetDirection(PIN_4, GPIO_PIN_DIRECTION_OUTPUT);
    BOARD_GPIO_5_DRV->SetValue(PIN_4, GPIO_PIN_OUTPUT_STATE_LOW);
    pinconf_set(PORT_5, PIN_4, PINMUX_ALTERNATE_FUNCTION_0, config_gpio);
}

/**
 * @brief Generates a random tensor.
 *
 * This function populates the input tensor with random values.
 *
 * @param tensor Pointer to the input tensor.
 */
void initialize_random_tensor(TfLiteTensor* tensor) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> dis(0, 255);

    auto dataPtr = tensor->data.uint8;
    const size_t tensorSize = tensor->bytes;

    for (size_t i = 0; i < tensorSize; ++i) {
        dataPtr[i] = dis(gen);
    }
}

int main()
{
    /* Initialise the UART module to allow printf related functions (if using retarget) */
    BoardInit();
    initialize_gpio_pins();
    info("Hello World!\n");

    /* Model object creation and initialisation. */
    arm::app::TestModel model;  /* Model wrapper object. */

    /* Load the model. */
    if (!model.Init(arm::app::tensorArena,
                    sizeof(arm::app::tensorArena),
                    arm::app::object_detection::GetModelPointer(),
                    arm::app::object_detection::GetModelLen())) {
        printf_err("Failed to initialise model\n");
        return 1;
    }

    TfLiteTensor* inputTensor   = model.GetInputTensor(0);
    TfLiteTensor* outputTensor0 = model.GetOutputTensor(0);
    TfLiteTensor* outputTensor1 = model.GetOutputTensor(1);

    if (!inputTensor->dims) {
        printf_err("Invalid input tensor dims\n");
        return 1;
    } else if (inputTensor->dims->size < 3) {
        printf_err("Input tensor dimension should be >= 3\n");
        return 1;
    }

    /* Initialize the input tensor with random values. */
    initialize_random_tensor(inputTensor);

    /* Run inference over the random tensor. */
    info("Running inference on random tensor\n");
    BOARD_GPIO_5_DRV->SetValue(PIN_4, GPIO_PIN_OUTPUT_STATE_HIGH);
    if (!model.RunInference()) {
        printf_err("Inference failed.\n");
        return 2;
    }
    BOARD_GPIO_5_DRV->SetValue(PIN_4, GPIO_PIN_OUTPUT_STATE_LOW);

    /* Post-process results if applicable. */
    // auto outputCount = model.GetOutputTensorCount();
    // for (int i = 0; i < outputCount; ++i) {
    //     TfLiteTensor* outputTensor = model.GetOutputTensor(i);
    //     info("Output tensor %d: size=%d\n", i, outputTensor->bytes);
    // }

    /* Log the results. */
    info("Inference completed successfully.\n");

    return 0;
}
