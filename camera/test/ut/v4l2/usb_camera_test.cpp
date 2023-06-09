/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include "usb_camera_test.h"

bool g_usbCameraExit = false;

void UtestUSBCameraTest::SetUpTestCase(void)
{}
void UtestUSBCameraTest::TearDownTestCase(void)
{}
void UtestUSBCameraTest::SetUp(void)
{
    if (display_ == nullptr)
    display_ = std::make_shared<TestDisplay>();
    display_->UsbInit();
}
void UtestUSBCameraTest::TearDown(void)
{
    display_->Close();
}

/**
  * @tc.name: USB Camera
  * @tc.desc: USB Camera, getCameraID success.
  * @tc.level: Level0
  * @tc.size: MediumTest
  * @tc.type: Function
  */
TEST_F(UtestUSBCameraTest, camera_usb_0001)
{
    uint32_t rc = 0;
    std::cout << "==========[test log] USB Camera, getCameraID success."<< std::endl;
    std::vector<std::string> cameraIds;
    std::cout << "==========[test log] 1. get current system cameraID."<< std::endl;
    display_->cameraHost->GetCameraIds(cameraIds);
    std::cout << "==========[test log] First cameraId.size = " << cameraIds.size() << std::endl;
    std::cout << "==========[test log] OnCameraStatus interface has been mobilized" << std::endl;
    for (const auto &cameraId : cameraIds) {
        std::cout << "==========[test log] cameraId = " << cameraId << std::endl;
    }
    std::cout << "==========[test log] 2. please add or delete the usb camera, wait for 10s..."<< std::endl;
    sleep(3); // judging add or delete the usb camera, wait for 3s.
    std::cout << "==========[test log] r u ready? wait for 10s..."<< std::endl;
    sleep(3); // judging r u ready, wait for 3s.
    std::cout << "==========[test log] 3. check the cameraID again... wait for 10s..."<< std::endl;
    sleep(3); // checking the cameraID again, wait for 3s.
    std::cout << "==========[test log] Second cameraId.size = " << cameraIds.size() << std::endl;
    if (cameraIds.size() == 1) {
        cameraIds.clear();
    }
    rc = display_->cameraHost->GetCameraIds(cameraIds);
    EXPECT_EQ(rc, HDI::Camera::V1_0::NO_ERROR);
    for (const auto &cameraId : cameraIds) {
        std::cout << "cameraId = " << cameraId << std::endl;
    }
    if (cameraIds.size() > 1) {
        g_usbCameraExit = true;
    }
}

/**
  * @tc.name: USB Camera
  * @tc.desc: get value of OHOS_ABILITY_ZOOM_RATIO_RANGE
  * @tc.level: Level0
  * @tc.size: MediumTest
  * @tc.type: Function
  */
TEST_F(UtestUSBCameraTest, camera_usb_0002)
{
    if (!g_usbCameraExit) {
        GTEST_SKIP() << "No usb camera plugged in" << std::endl;
    }
    ability_ = display_->GetCameraAbility();
    EXPECT_NE(ability_, nullptr);
    common_metadata_header_t *data = ability_->get();
    camera_metadata_item_t entry;
    int ret = FindCameraMetadataItem(data, OHOS_ABILITY_ZOOM_RATIO_RANGE, &entry);
    if (ret == CAM_META_SUCCESS) {
        std::cout << "OHOS_ABILITY_ZOOM_RATIO_RANGE: count is " << (int)entry.count << std::endl;
        std::cout << "Zoom ratio range: [" << entry.data.f[0];
        std::cout << "," << entry.data.f[1] << "]" << std::endl;
    } else if (ret == CAM_META_ITEM_NOT_FOUND) {
        std::cout << "OHOS_ABILITY_ZOOM_RATIO_RANGE is not support" << std::endl;
    }
}

/**
  * @tc.name: USB Camera
  * @tc.desc: get value of OHOS_ABILITY_CAMERA_CONNECTION_TYPE
  * @tc.level: Level0
  * @tc.size: MediumTest
  * @tc.type: Function
  */
TEST_F(UtestUSBCameraTest, camera_usb_0003)
{
    if (!g_usbCameraExit) {
        GTEST_SKIP() << "No usb camera plugged in" << std::endl;
    }
    ability_ = display_->GetCameraAbility();
    EXPECT_NE(ability_, nullptr);
    common_metadata_header_t *data = ability_->get();
    camera_metadata_item_t entry;
    int ret = FindCameraMetadataItem(data, OHOS_ABILITY_CAMERA_CONNECTION_TYPE, &entry);
    EXPECT_EQ(ret, CAM_META_SUCCESS);
    std::cout << "OHOS_ABILITY_CAMERA_CONNECTION_TYPE value is "
        << static_cast<int>(entry.data.u8[0]) << std::endl;
    EXPECT_TRUE(entry.data.u8[0] == OHOS_CAMERA_CONNECTION_TYPE_USB_PLUGIN);
}

/**
  * @tc.name: USB Camera
  * @tc.desc: get value of OHOS_ABILITY_CAMERA_POSITION
  * @tc.level: Level0
  * @tc.size: MediumTest
  * @tc.type: Function
  */
TEST_F(UtestUSBCameraTest, camera_usb_0004)
{
    if (!g_usbCameraExit) {
        GTEST_SKIP() << "No usb camera plugged in" << std::endl;
    }
    ability_ = display_->GetCameraAbility();
    EXPECT_NE(ability_, nullptr);
    common_metadata_header_t *data = ability_->get();
    camera_metadata_item_t entry;
    int ret = FindCameraMetadataItem(data, OHOS_ABILITY_CAMERA_POSITION, &entry);
    EXPECT_EQ(ret, CAM_META_SUCCESS);
    std::cout << "OHOS_ABILITY_CAMERA_POSITION value is " << static_cast<int>(entry.data.u8[0]) << std::endl;
    EXPECT_TRUE(entry.data.u8[0] == OHOS_CAMERA_POSITION_OTHER);
}

/**
  * @tc.name: USB Camera
  * @tc.desc: get value of OHOS_ABILITY_FLASH_AVAILABLE
  * @tc.level: Level0
  * @tc.size: MediumTest
  * @tc.type: Function
  */
TEST_F(UtestUSBCameraTest, camera_usb_0005)
{
    if (!g_usbCameraExit) {
        GTEST_SKIP() << "No usb camera plugged in" << std::endl;
    }
    ability_ = display_->GetCameraAbility();
    EXPECT_NE(ability_, nullptr);
    common_metadata_header_t *data = ability_->get();
    camera_metadata_item_t entry;
    int ret = FindCameraMetadataItem(data, OHOS_ABILITY_FLASH_AVAILABLE, &entry);
    EXPECT_EQ(ret, CAM_META_SUCCESS);
    std::cout << "OHOS_ABILITY_FLASH_AVAILABLE value is " << static_cast<int>(entry.data.u8[0]) << std::endl;
}

/**
  * @tc.name: USB Camera
  * @tc.desc: get value of OHOS_ABILITY_VIDEO_STABILIZATION_MODES
  * @tc.level: Level0
  * @tc.size: MediumTest
  * @tc.type: Function
  */
TEST_F(UtestUSBCameraTest, camera_usb_0006)
{
    if (!g_usbCameraExit) {
        GTEST_SKIP() << "No usb camera plugged in" << std::endl;
    }
    ability_ = display_->GetCameraAbility();
    EXPECT_NE(ability_, nullptr);
    common_metadata_header_t *data = ability_->get();
    camera_metadata_item_t entry;
    int ret = FindCameraMetadataItem(data, OHOS_ABILITY_VIDEO_STABILIZATION_MODES, &entry);
    if (ret == CAM_META_SUCCESS) {
        for (int i = 0; i < entry.count; i++) {
            std::cout << "OHOS_ABILITY_VIDEO_STABILIZATION_MODES value is "
                    << static_cast<int>(entry.data.u8[i]) << std::endl;
        }
    } else if (ret == CAM_META_ITEM_NOT_FOUND) {
        std::cout << "OHOS_ABILITY_VIDEO_STABILIZATION_MODES is not support" << std::endl;
    }
}

/**
  * @tc.name: USB Camera
  * @tc.desc: get value of OHOS_ABILITY_FLASH_MODES
  * @tc.level: Level0
  * @tc.size: MediumTest
  * @tc.type: Function
  */
TEST_F(UtestUSBCameraTest, camera_usb_007)
{
    if (!g_usbCameraExit) {
        GTEST_SKIP() << "No usb camera plugged in" << std::endl;
    }
    ability_ = display_->GetCameraAbility();
    common_metadata_header_t *data = ability_->get();
    camera_metadata_item_t entry;
    int ret = FindCameraMetadataItem(data, OHOS_ABILITY_FLASH_MODES, &entry);
    if (ret == CAM_META_SUCCESS) {
        std::cout << "supported flash mode list:";
        for (int i = 0; i < entry.count; i++) {
            std::cout << " " << static_cast<int>(entry.data.u8[i]);
        }
        std::cout << std::endl;
    } else if (ret == CAM_META_ITEM_NOT_FOUND) {
        std::cout << "OHOS_ABILITY_FLASH_MODES is not support" << std::endl;
    }
}

/**
  * @tc.name: USB Camera
  * @tc.desc: get value of OHOS_ABILITY_FOCUS_MODES
  * @tc.level: Level0
  * @tc.size: MediumTest
  * @tc.type: Function
  */
TEST_F(UtestUSBCameraTest, camera_usb_008)
{
    if (!g_usbCameraExit) {
        GTEST_SKIP() << "No usb camera plugged in" << std::endl;
    }
    ability_ = display_->GetCameraAbility();
    EXPECT_NE(ability_, nullptr);
    common_metadata_header_t *data = ability_->get();
    camera_metadata_item_t entry;
    int ret = FindCameraMetadataItem(data, OHOS_ABILITY_FOCUS_MODES, &entry);
    if (ret == CAM_META_SUCCESS) {
        std::cout << "supported focus mode list:";
        for (int i = 0; i < entry.count; i++) {
            std::cout << " " << static_cast<int>(entry.data.u8[i]);
        }
        std::cout << std::endl;
    } else if (ret == CAM_META_ITEM_NOT_FOUND) {
        std::cout << "OHOS_ABILITY_FOCUS_MODES is not support" << std::endl;
    }
}

/**
  * @tc.name: USB Camera
  * @tc.desc: get value of OHOS_ABILITY_EXPOSURE_MODES
  * @tc.level: Level0
  * @tc.size: MediumTest
  * @tc.type: Function
  */
TEST_F(UtestUSBCameraTest, camera_usb_009)
{
    if (!g_usbCameraExit) {
        GTEST_SKIP() << "No usb camera plugged in" << std::endl;
    }
    ability_ = display_->GetCameraAbility();
    EXPECT_NE(ability_, nullptr);
    common_metadata_header_t *data = ability_->get();
    camera_metadata_item_t entry;
    int ret = FindCameraMetadataItem(data, OHOS_ABILITY_EXPOSURE_MODES, &entry);
    if (ret == CAM_META_SUCCESS) {
        std::cout << "supported exposure mode list:";
        for (int i = 0; i < entry.count; i++) {
            std::cout << " " << static_cast<int>(entry.data.u8[i]);
        }
        std::cout << std::endl;
    } else if (ret == CAM_META_ITEM_NOT_FOUND) {
        std::cout << "OHOS_ABILITY_EXPOSURE_MODES is not support" << std::endl;
    }
}

/**
  * @tc.name: USB Camera
  * @tc.desc: get value of OHOS_CONTROL_CAPTURE_MIRROR_SUPPORTED
  * @tc.level: Level0
  * @tc.size: MediumTest
  * @tc.type: Function
  */
TEST_F(UtestUSBCameraTest, camera_usb_0010)
{
    if (!g_usbCameraExit) {
        GTEST_SKIP() << "No usb camera plugged in" << std::endl;
    }
    ability_ = display_->GetCameraAbility();
    EXPECT_NE(ability_, nullptr);
    common_metadata_header_t *data = ability_->get();
    camera_metadata_item_t entry;
    int ret = FindCameraMetadataItem(data, OHOS_CONTROL_CAPTURE_MIRROR_SUPPORTED, &entry);
    EXPECT_EQ(ret, CAM_META_SUCCESS);
    std::cout << "capture mirror supported is :";
    for (int i = 0; i < entry.count; i++) {
        std::cout << " " << static_cast<int>(entry.data.u8[i]);
    }
    std::cout << std::endl;
}

/**
  * @tc.name: USB Camera
  * @tc.desc: get value of OHOS_ABILITY_MUTE_MODES
  * @tc.level: Level0
  * @tc.size: MediumTest
  * @tc.type: Function
  */
TEST_F(UtestUSBCameraTest, camera_usb_0011)
{
    if (!g_usbCameraExit) {
        GTEST_SKIP() << "No usb camera plugged in" << std::endl;
    }
    ability_ = display_->GetCameraAbility();
    EXPECT_NE(ability_, nullptr);
    common_metadata_header_t *data = ability_->get();
    camera_metadata_item_t entry;
    int ret = FindCameraMetadataItem(data, OHOS_ABILITY_MUTE_MODES, &entry);
    if (ret == CAM_META_SUCCESS) {
        std::cout << "supported mute mode is:";
        for (int i = 0; i < entry.count; i++) {
            std::cout << " " << static_cast<int>(entry.data.u8[i]);
        }
        std::cout << std::endl;
    } else if (ret == CAM_META_ITEM_NOT_FOUND) {
        std::cout << "OHOS_ABILITY_MUTE_MODES is not support" << std::endl;
    }
}

/**
  * @tc.name: USB Camera
  * @tc.desc: get value of OHOS_ABILITY_FPS_RANGES
  * @tc.level: Level0
  * @tc.size: MediumTest
  * @tc.type: Function
  */
TEST_F(UtestUSBCameraTest, camera_usb_0012)
{
    if (!g_usbCameraExit) {
        GTEST_SKIP() << "No usb camera plugged in" << std::endl;
    }
    ability_ = display_->GetCameraAbility();
    EXPECT_NE(ability_, nullptr);
    common_metadata_header_t *data = ability_->get();
    camera_metadata_item_t entry;
    int ret = FindCameraMetadataItem(data, OHOS_ABILITY_FPS_RANGES, &entry);
    EXPECT_EQ(ret, CAM_META_SUCCESS);
    std::cout << "supported fps ranges list: [";
    std::cout << static_cast<int>(entry.data.i32[0]) << "," << static_cast<int>(entry.data.i32[1]) << "]";
    std::cout << std::endl;
}

/**
  * @tc.name: USB Camera
  * @tc.desc: get value of OHOS_ABILITY_CAMERA_TYPE
  * @tc.level: Level0
  * @tc.size: MediumTest
  * @tc.type: Function
  */
TEST_F(UtestUSBCameraTest, camera_usb_0013)
{
    if (!g_usbCameraExit) {
        GTEST_SKIP() << "No usb camera plugged in" << std::endl;
    }
    ability_ = display_->GetCameraAbility();
    EXPECT_NE(ability_, nullptr);
    common_metadata_header_t *data = ability_->get();
    camera_metadata_item_t entry;
    int ret = FindCameraMetadataItem(data, OHOS_ABILITY_CAMERA_TYPE, &entry);
    EXPECT_EQ(ret, CAM_META_SUCCESS);
    std::cout << "OHOS_ABILITY_CAMERA_TYPE value is " << static_cast<int>(entry.data.u8[0]) << std::endl;
    EXPECT_TRUE(entry.data.u8[0] == OHOS_CAMERA_TYPE_UNSPECIFIED);
}

/**
  * @tc.name: USB Camera
  * @tc.desc: get value of OHOS_JPEG_ORIENTATION
  * @tc.level: Level0
  * @tc.size: MediumTest
  * @tc.type: Function
  */
TEST_F(UtestUSBCameraTest, camera_usb_0014)
{
    if (!g_usbCameraExit) {
        GTEST_SKIP() << "No usb camera plugged in" << std::endl;
    }
    ability_ = display_->GetCameraAbility();
    EXPECT_NE(ability_, nullptr);
    common_metadata_header_t *data = ability_->get();
    camera_metadata_item_t entry;
    int ret = FindCameraMetadataItem(data, OHOS_JPEG_ORIENTATION, &entry);
    EXPECT_EQ(ret, CAM_META_SUCCESS);
    std::cout << "OHOS_JPEG_ORIENTATION value is " << static_cast<int>(entry.data.i32[0]) << std::endl;
}

/**
  * @tc.name: USB Camera
  * @tc.desc: get value of OHOS_JPEG_QUALITY
  * @tc.level: Level0
  * @tc.size: MediumTest
  * @tc.type: Function
  */
TEST_F(UtestUSBCameraTest, camera_usb_0015)
{
    if (!g_usbCameraExit) {
        GTEST_SKIP() << "No usb camera plugged in" << std::endl;
    }
    ability_ = display_->GetCameraAbility();
    EXPECT_NE(ability_, nullptr);
    common_metadata_header_t *data = ability_->get();
    camera_metadata_item_t entry;
    int ret = FindCameraMetadataItem(data, OHOS_JPEG_QUALITY, &entry);
    EXPECT_EQ(ret, CAM_META_SUCCESS);
    std::cout << "OHOS_JPEG_ORIENTATION value is " << static_cast<int>(entry.data.u8[0]) << std::endl;
}

/**
  * @tc.name: USB Camera
  * @tc.desc: get value of OHOS_ABILITY_STREAM_AVAILABLE_BASIC_CONFIGURATIONS
  * @tc.level: Level0
  * @tc.size: MediumTest
  * @tc.type: Function
  */
TEST_F(UtestUSBCameraTest, camera_usb_0016)
{
    if (!g_usbCameraExit) {
        GTEST_SKIP() << "No usb camera plugged in" << std::endl;
    }
    ability_ = display_->GetCameraAbility();
    EXPECT_NE(ability_, nullptr);
    common_metadata_header_t *data = ability_->get();
    camera_metadata_item_t entry;
    int ret = FindCameraMetadataItem(data, OHOS_ABILITY_STREAM_AVAILABLE_BASIC_CONFIGURATIONS, &entry);
    EXPECT_EQ(ret, CAM_META_SUCCESS);
    std::cout << "OHOS_ABILITY_STREAM_AVAILABLE_BASIC_CONFIGURATIONS value is "
        << static_cast<int>(entry.data.u8[0]) << std::endl;
    EXPECT_TRUE(entry.data.u8[0] == OHOS_CAMERA_FORMAT_RGBA_8888);
}

/**
  * @tc.name: USB Camera
  * @tc.desc: get value of OHOS_ABILITY_STREAM_AVAILABLE_EXTEND_CONFIGURATIONS
  * @tc.level: Level0
  * @tc.size: MediumTest
  * @tc.type: Function
  */
TEST_F(UtestUSBCameraTest, camera_usb_0017)
{
    if (!g_usbCameraExit) {
        GTEST_SKIP() << "No usb camera plugged in" << std::endl;
    }
    ability_ = display_->GetCameraAbility();
    EXPECT_NE(ability_, nullptr);
    common_metadata_header_t *data = ability_->get();
    EXPECT_NE(data, nullptr);
    camera_metadata_item_t entry;
    int ret = FindCameraMetadataItem(data, OHOS_ABILITY_STREAM_AVAILABLE_EXTEND_CONFIGURATIONS, &entry);
    if (ret == 0 && entry.data.i32 != nullptr && entry.count > 0) {
        std::cout << "print tag<OHOS_ABILITY_STREAM_AVAILABLE_EXTEND_CONFIGURATIONS> value start." << std::endl;
        const size_t STEP = 10; // print step
        std::cout << "count" << entry.count << std::endl;
        for (size_t i = 0; i < entry.count; i++) {
            std::cout << entry.data.i32[i] << " ";
            if ((i != 0) && (i % STEP == 0 || i == entry.count - 1)) {
                std::cout << std::endl;
            }
        }
        std::cout << "print tag<OHOS_ABILITY_STREAM_AVAILABLE_EXTEND_CONFIGURATIONS> value end." << std::endl;
    }
}

/**
  * @tc.name: USB Camera
  * @tc.desc: Preview stream, expected success.
  * @tc.level: Level0
  * @tc.size: MediumTest
  * @tc.type: Function
  */
TEST_F(UtestUSBCameraTest, camera_usb_0018)
{
    if (!g_usbCameraExit) {
        GTEST_SKIP() << "No usb camera plugged in" << std::endl;
    }
    // Get the device manager
    display_->OpenUsbCamera();
    // Get the stream manager
    display_->AchieveStreamOperator();
    // start stream
    display_->intents = {PREVIEW};
    display_->StartStream(display_->intents);
    // Get preview
    display_->StartCapture(display_->STREAM_ID_PREVIEW, display_->CAPTURE_ID_PREVIEW, false, true);
    // release stream
    display_->captureIds = {display_->CAPTURE_ID_PREVIEW};
    display_->streamIds = {display_->STREAM_ID_PREVIEW};
    display_->StopStream(display_->captureIds, display_->streamIds);
}

/**
  * @tc.name: USB Camera
  * @tc.desc: Preview stream, width = 1280, height = 720, expected success.
  * @tc.level: Level0
  * @tc.size: MediumTest
  * @tc.type: Function
  */
TEST_F(UtestUSBCameraTest, camera_usb_0019)
{
    if (!g_usbCameraExit) {
        GTEST_SKIP() << "No usb camera plugged in" << std::endl;
    }
    // Get the device manager
    display_->OpenUsbCamera();
    // Create and get streamOperator information
    display_->AchieveStreamOperator();
    // Create data stream
    if (display_->streamCustomerPreview_ == nullptr) {
        display_->streamCustomerPreview_ = std::make_shared<StreamCustomer>();
    }
    OHOS::sptr<OHOS::IBufferProducer> producer = display_->streamCustomerPreview_->CreateProducer();
    producer->SetQueueSize(8); // 8:set bufferQueue size

    std::vector<StreamInfo> streamInfos;
    StreamInfo streamInfo = {};
    streamInfo.streamId_ = display_->STREAM_ID_PREVIEW;
    streamInfo.width_ = 1280; // 1280:picture width
    streamInfo.height_ = 720; // 720:picture height
    streamInfo.format_ = PIXEL_FMT_RGBA_8888;
    streamInfo.dataspace_ = 8; // 8:picture dataspace
    streamInfo.intent_ = PREVIEW;
    streamInfo.tunneledMode_ = 5; // 5:tunnel mode
    streamInfo.bufferQueue_ = new BufferProducerSequenceable(producer);
    ASSERT_NE(streamInfo.bufferQueue_, nullptr);
    std::vector<StreamInfo>().swap(streamInfos);
    streamInfos.push_back(streamInfo);
    display_->rc = (CamRetCode)display_->streamOperator->CreateStreams(streamInfos);
    EXPECT_EQ(true, display_->rc == HDI::Camera::V1_0::NO_ERROR);
    // Submit stream information
    display_->rc = (CamRetCode)display_->streamOperator->CommitStreams(NORMAL, display_->ability_);
    EXPECT_EQ(true, display_->rc == HDI::Camera::V1_0::NO_ERROR);
    // capture
    display_->StartCapture(display_->STREAM_ID_PREVIEW, display_->CAPTURE_ID_PREVIEW, false, true);
    // release stream
    display_->captureIds = {display_->CAPTURE_ID_PREVIEW};
    display_->streamIds = {display_->STREAM_ID_PREVIEW};
    display_->StopStream(display_->captureIds, display_->streamIds);
}

/**
  * @tc.name: USB Camera
  * @tc.desc: UpdateSettings, fps.
  * @tc.level: Level0
  * @tc.size: MediumTest
  * @tc.type: Function
  */
TEST_F(UtestUSBCameraTest, camera_usb_0020)
{
    if (!g_usbCameraExit) {
        GTEST_SKIP() << "No usb camera plugged in" << std::endl;
    }
    // Get the device manager
    display_->OpenUsbCamera();
    // get the stream manager
    display_->AchieveStreamOperator();

    // start stream
    display_->intents = {PREVIEW};
    display_->StartStream(display_->intents);

    // updateSettings
    const uint32_t ITEM_CAPACITY = 100;
    const uint32_t DATA_CAPACITY = 2000;
    const int32_t FPS_VALUE = 10;
    std::shared_ptr<CameraSetting> meta = std::make_shared<CameraSetting>(
        ITEM_CAPACITY, DATA_CAPACITY);
    std::vector<int32_t> fpsRange;
    fpsRange.push_back(FPS_VALUE);
    fpsRange.push_back(FPS_VALUE);
    meta->addEntry(OHOS_CONTROL_FPS_RANGES, fpsRange.data(), fpsRange.size());
    const int32_t DEVICE_STREAM_ID = 0;
    meta->addEntry(OHOS_CAMERA_STREAM_ID, &DEVICE_STREAM_ID, 1);
    std::vector<uint8_t> setting;
    MetadataUtils::ConvertMetadataToVec(meta, setting);
    display_->rc = (CamRetCode)display_->cameraDevice->UpdateSettings(setting);
    EXPECT_EQ(true, display_->rc == HDI::Camera::V1_0::NO_ERROR);

    // get preview
    display_->StartCapture(display_->STREAM_ID_PREVIEW, display_->CAPTURE_ID_PREVIEW, false, true);

    // release stream
    display_->captureIds = {display_->CAPTURE_ID_PREVIEW};
    display_->streamIds = {display_->STREAM_ID_PREVIEW};
    display_->StopStream(display_->captureIds, display_->streamIds);
}

/**
  * @tc.name: USB Camera
  * @tc.desc: USB Camera, OnCameraStatus and OnCameraEvent.
  * @tc.level: Level0
  * @tc.size: MediumTest
  * @tc.type: Function
  */
TEST_F(UtestUSBCameraTest, camera_usb_0021)
{
    uint32_t rc = 0;
    std::cout << "==========[test log] USB Camera, getCameraID success."<< std::endl;
    std::vector<std::string> cameraIds;
    std::cout << "==========[test log] 1. get current system cameraID."<< std::endl;
    display_->cameraHost->GetCameraIds(cameraIds);
    std::cout << "==========[test log] First cameraId.size = " << cameraIds.size() << std::endl;
    std::cout << "==========[test log] OnCameraStatus interface has been mobilized" << std::endl;
    for (const auto &cameraId : cameraIds) {
        std::cout << "==========[test log] cameraId = " << cameraId << std::endl;
    }
    const int count = 4;
    for (int i = 0; i < count; i++) {
        std::cout << "==========[test log] 2. please add or delete the usb camera, wait for 3s..."<< std::endl;
        sleep(3); // judging add or delete the usb camera, wait for 3s.
    }
    std::cout << "==========[test log] 3. check the cameraID again... wait for 3s..."<< std::endl;
    sleep(3); // checking the cameraID again, wait for 3s.
    std::cout << "==========[test log] Second cameraId.size = " << cameraIds.size() << std::endl;
    if (cameraIds.size() == 1) {
        cameraIds.clear();
    }
    rc = display_->cameraHost->GetCameraIds(cameraIds);
    EXPECT_EQ(rc, HDI::Camera::V1_0::NO_ERROR);
    for (const auto &cameraId : cameraIds) {
        std::cout << "cameraId = " << cameraId << std::endl;
    }
}

/**
  * @tc.name: USB Camera
  * @tc.desc: Commit 2 streams together, Preview and still_capture streams, isStreaming is true.
  * @tc.level: Level0
  * @tc.size: MediumTest
  * @tc.type: Function
  */
TEST_F(UtestUSBCameraTest, camera_usb_0022)
{
    // Get the device manager
    display_->OpenUsbCamera();
    if (!g_usbCameraExit) {
        GTEST_SKIP() << "No usb camera plugged in" << std::endl;
    }
    // Get the stream manager
    display_->AchieveStreamOperator();
    // start stream
    display_->intents = {PREVIEW, STILL_CAPTURE};
    display_->StartStream(display_->intents);
    // Get preview
    display_->StartCapture(display_->STREAM_ID_PREVIEW, display_->CAPTURE_ID_PREVIEW, false, true);
    display_->StartCapture(display_->STREAM_ID_CAPTURE, display_->CAPTURE_ID_CAPTURE, false, true);
    // release stream
    display_->captureIds = {display_->CAPTURE_ID_PREVIEW, display_->CAPTURE_ID_CAPTURE};
    display_->streamIds = {display_->STREAM_ID_PREVIEW, display_->STREAM_ID_CAPTURE};
    display_->StopStream(display_->captureIds, display_->streamIds);
}

/**
  * @tc.name: USB Camera
  * @tc.desc: Commit 2 streams together, width = 1280, height = 720, expected success.
  * @tc.level: Level0
  * @tc.size: MediumTest
  * @tc.type: Function
  */
TEST_F(UtestUSBCameraTest, camera_usb_0023)
{
    display_->OpenUsbCamera();
    if (!g_usbCameraExit) {
        GTEST_SKIP() << "No usb camera plugged in" << std::endl;
    }
    display_->AchieveStreamOperator();
    if (display_->streamCustomerPreview_ == nullptr) {
        display_->streamCustomerPreview_ = std::make_shared<StreamCustomer>();
    }
    std::vector<StreamInfo> streamInfos;
    StreamInfo streamInfo = {};
    streamInfo.streamId_ = display_->STREAM_ID_PREVIEW;
    streamInfo.width_ = 1280; // 1280:picture width
    streamInfo.height_ = 720; // 720:picture height
    streamInfo.format_ = PIXEL_FMT_RGBA_8888;
    streamInfo.dataspace_ = 8; // 8:picture dataspace
    streamInfo.intent_ = PREVIEW;
    streamInfo.tunneledMode_ = 5; // 5:tunnel mode
    streamInfo.bufferQueue_ = new BufferProducerSequenceable(display_->streamCustomerPreview_->CreateProducer());
    ASSERT_NE(streamInfo.bufferQueue_, nullptr);
    streamInfo.bufferQueue_->producer_->SetQueueSize(8); // 8:set bufferQueue size
    streamInfos.push_back(streamInfo);
    if (display_->streamCustomerCapture_ == nullptr) {
        display_->streamCustomerCapture_ = std::make_shared<StreamCustomer>();
    }
    StreamInfo streamInfoCapture = {};
    streamInfoCapture.streamId_ = display_->STREAM_ID_CAPTURE;
    streamInfoCapture.width_ = 1280; // 1280:picture width
    streamInfoCapture.height_ = 960; // 960:picture height
    streamInfoCapture.format_ = PIXEL_FMT_RGBA_8888;
    streamInfoCapture.dataspace_ = 8; // 8:picture dataspace
    streamInfoCapture.intent_ = STILL_CAPTURE;
    streamInfoCapture.encodeType_ = ENCODE_TYPE_JPEG;
    streamInfoCapture.tunneledMode_ = 5; // 5:tunnel mode
    streamInfoCapture.bufferQueue_ = new BufferProducerSequenceable(display_->streamCustomerCapture_->CreateProducer());
    ASSERT_NE(streamInfoCapture.bufferQueue_, nullptr);
    streamInfoCapture.bufferQueue_->producer_->SetQueueSize(8); // 8:set bufferQueue size
    streamInfos.push_back(streamInfoCapture);
    display_->rc = (CamRetCode)display_->streamOperator->CreateStreams(streamInfos);
    EXPECT_EQ(true, display_->rc == HDI::Camera::V1_0::NO_ERROR);
    display_->rc = (CamRetCode)display_->streamOperator->CommitStreams(NORMAL, display_->ability_);
    EXPECT_EQ(true, display_->rc == HDI::Camera::V1_0::NO_ERROR);
    display_->StartCapture(display_->STREAM_ID_PREVIEW, display_->CAPTURE_ID_PREVIEW, false, true);
    display_->StartCapture(display_->STREAM_ID_CAPTURE, display_->CAPTURE_ID_CAPTURE, false, true);
    display_->captureIds = {display_->CAPTURE_ID_PREVIEW, display_->CAPTURE_ID_CAPTURE};
    display_->streamIds = {display_->STREAM_ID_PREVIEW, display_->STREAM_ID_CAPTURE};
    display_->StopStream(display_->captureIds, display_->streamIds);
}

/**
  * @tc.name: preview and capture
  * @tc.desc: Commit 2 streams together, Change the value OHOS_JPEG_ORIENTATION, isStreaming is true.
  * @tc.level: Level0
  * @tc.size: MediumTest
  * @tc.type: Function
  */
TEST_F(UtestUSBCameraTest, camera_usb_0024)
{
    // Get the device manager
    display_->OpenUsbCamera();
    if (!g_usbCameraExit) {
        GTEST_SKIP() << "No usb camera plugged in" << std::endl;
    }
    // Get the stream manager
    display_->AchieveStreamOperator();
    std::vector<int32_t> jpegOrientationVector;
    jpegOrientationVector.push_back(OHOS_CAMERA_JPEG_ROTATION_0);
    display_->ability->updateEntry(OHOS_JPEG_ORIENTATION, jpegOrientationVector.data(), jpegOrientationVector.size());
    display_->ability_.clear();
    MetadataUtils::ConvertMetadataToVec(display_->ability, display_->ability_);
    // start stream
    display_->intents = {PREVIEW, STILL_CAPTURE};
    display_->StartStream(display_->intents);

    // Get preview
    display_->StartCapture(display_->STREAM_ID_PREVIEW, display_->CAPTURE_ID_PREVIEW, false, true);
    display_->StartCapture(display_->STREAM_ID_CAPTURE, display_->CAPTURE_ID_CAPTURE, false, true);
    // release stream
    display_->captureIds = {display_->CAPTURE_ID_PREVIEW, display_->CAPTURE_ID_CAPTURE};
    display_->streamIds = {display_->STREAM_ID_PREVIEW, display_->STREAM_ID_CAPTURE};
    display_->StopStream(display_->captureIds, display_->streamIds);
}

/**
  * @tc.name: preview and capture
  * @tc.desc: Commit 2 streams together, Preview and still_capture streams, isStreaming is true.
  * @tc.level: Level0
  * @tc.size: MediumTest
  * @tc.type: Function
  */
TEST_F(UtestUSBCameraTest, camera_usb_0025)
{
    // Get the device manager
    display_->OpenUsbCamera();
    if (!g_usbCameraExit) {
        GTEST_SKIP() << "No usb camera plugged in" << std::endl;
    }
    // Get the stream manager
    display_->AchieveStreamOperator();
    std::vector<int32_t> jpegQualityVector;
    jpegQualityVector.push_back(OHOS_CAMERA_JPEG_LEVEL_LOW);
    display_->ability->updateEntry(OHOS_JPEG_QUALITY, jpegQualityVector.data(), jpegQualityVector.size());
    display_->ability_.clear();
    MetadataUtils::ConvertMetadataToVec(display_->ability, display_->ability_);
    // start stream
    display_->intents = {PREVIEW, STILL_CAPTURE};
    display_->StartStream(display_->intents);

    // Get preview
    display_->StartCapture(display_->STREAM_ID_PREVIEW, display_->CAPTURE_ID_PREVIEW, false, true);
    display_->StartCapture(display_->STREAM_ID_CAPTURE, display_->CAPTURE_ID_CAPTURE, false, true);
    // release stream
    display_->captureIds = {display_->CAPTURE_ID_PREVIEW, display_->CAPTURE_ID_CAPTURE};
    display_->streamIds = {display_->STREAM_ID_PREVIEW, display_->STREAM_ID_CAPTURE};
    display_->StopStream(display_->captureIds, display_->streamIds);
}

/**
  * @tc.name: Video
  * @tc.desc: Preview + video, commit together, success.
  * @tc.level: Level0
  * @tc.size: MediumTest
  * @tc.type: Function
  */
TEST_F(UtestUSBCameraTest, camera_usb_0026)
{
    // Get the device manager
    display_->OpenUsbCamera();
    if (!g_usbCameraExit) {
        GTEST_SKIP() << "No usb camera plugged in" << std::endl;
    }
    // Create and get streamOperator information
    display_->AchieveStreamOperator();
    // start stream
    display_->intents = {PREVIEW, VIDEO};
    display_->StartStream(display_->intents);
    // Get preview
    display_->StartCapture(display_->STREAM_ID_PREVIEW, display_->CAPTURE_ID_PREVIEW, false, true);
    display_->StartCapture(display_->STREAM_ID_VIDEO, display_->CAPTURE_ID_VIDEO, false, true);

    display_->captureIds = {display_->CAPTURE_ID_PREVIEW, display_->CAPTURE_ID_VIDEO};
    display_->streamIds = {display_->STREAM_ID_PREVIEW, display_->STREAM_ID_VIDEO};
    display_->StopStream(display_->captureIds, display_->streamIds);
}

/**
  * @tc.name: USB Camera
  * @tc.desc: Preview stream, width = 1280, height = 720, expected success.
  * @tc.level: Level0
  * @tc.size: MediumTest
  * @tc.type: Function
  */
TEST_F(UtestUSBCameraTest, camera_usb_0027)
{
    display_->OpenUsbCamera();
    if (!g_usbCameraExit) {
        GTEST_SKIP() << "No usb camera plugged in" << std::endl;
    }
    display_->AchieveStreamOperator();
    if (display_->streamCustomerPreview_ == nullptr) {
        display_->streamCustomerPreview_ = std::make_shared<StreamCustomer>();
    }
    std::vector<StreamInfo> streamInfos;
    StreamInfo streamInfo = {};
    streamInfo.streamId_ = display_->STREAM_ID_PREVIEW;
    streamInfo.width_ = 1280; // 1280:picture width
    streamInfo.height_ = 720; // 720:picture height
    streamInfo.format_ = PIXEL_FMT_RGBA_8888;
    streamInfo.dataspace_ = 8; // 8:picture dataspace
    streamInfo.intent_ = PREVIEW;
    streamInfo.tunneledMode_ = 5; // 5:tunnel mode
    streamInfo.bufferQueue_ = new BufferProducerSequenceable(display_->streamCustomerPreview_->CreateProducer());
    ASSERT_NE(streamInfo.bufferQueue_, nullptr);
    streamInfo.bufferQueue_->producer_->SetQueueSize(8); // 8:set bufferQueue size
    streamInfos.push_back(streamInfo);
    if (display_->streamCustomerVideo_ == nullptr) {
        display_->streamCustomerVideo_ = std::make_shared<StreamCustomer>();
    }
    StreamInfo streamInfoVideo = {};
    streamInfoVideo.streamId_ = display_->STREAM_ID_VIDEO;
    streamInfoVideo.width_ = 1280; // 1280:picture width
    streamInfoVideo.height_ = 960; // 960:picture height
    streamInfoVideo.format_ = PIXEL_FMT_RGBA_8888;
    streamInfoVideo.dataspace_ = 8; // 8:picture dataspace
    streamInfoVideo.intent_ = VIDEO;
    streamInfoVideo.encodeType_ = ENCODE_TYPE_H264;
    streamInfoVideo.tunneledMode_ = 5; // 5:tunnel mode
    streamInfoVideo.bufferQueue_ = new BufferProducerSequenceable(display_->streamCustomerVideo_->CreateProducer());
    ASSERT_NE(streamInfoVideo.bufferQueue_, nullptr);
    streamInfoVideo.bufferQueue_->producer_->SetQueueSize(8); // 8:set bufferQueue size
    streamInfos.push_back(streamInfoVideo);
    display_->rc = (CamRetCode)display_->streamOperator->CreateStreams(streamInfos);
    EXPECT_EQ(true, display_->rc == HDI::Camera::V1_0::NO_ERROR);
    display_->rc = (CamRetCode)display_->streamOperator->CommitStreams(NORMAL, display_->ability_);
    EXPECT_EQ(true, display_->rc == HDI::Camera::V1_0::NO_ERROR);
    display_->StartCapture(display_->STREAM_ID_PREVIEW, display_->CAPTURE_ID_PREVIEW, false, true);
    display_->StartCapture(display_->STREAM_ID_VIDEO, display_->CAPTURE_ID_VIDEO, false, true);
    display_->captureIds = {display_->CAPTURE_ID_PREVIEW, display_->CAPTURE_ID_VIDEO};
    display_->streamIds = {display_->STREAM_ID_PREVIEW, display_->STREAM_ID_VIDEO};
    display_->StopStream(display_->captureIds, display_->streamIds);
}

/**
  * @tc.name: USB Camera
  * @tc.desc: UpdateSettings, fps.
  * @tc.level: Level0
  * @tc.size: MediumTest
  * @tc.type: Function
  */
TEST_F(UtestUSBCameraTest, camera_usb_0028)
{
    // Get the device manager
    display_->OpenUsbCamera();
    if (!g_usbCameraExit) {
        GTEST_SKIP() << "No usb camera plugged in" << std::endl;
    }
    // get the stream manager
    display_->AchieveStreamOperator();

    // start stream
    display_->intents = {PREVIEW, VIDEO};
    display_->StartStream(display_->intents);

    // updateSettings
    const uint32_t ITEM_CAPACITY = 100;
    const uint32_t DATA_CAPACITY = 2000;
    const int32_t FPS_VALUE = 10;
    std::shared_ptr<CameraSetting> meta = std::make_shared<CameraSetting>(
        ITEM_CAPACITY, DATA_CAPACITY);
    std::vector<int32_t> fpsRange;
    fpsRange.push_back(FPS_VALUE);
    fpsRange.push_back(FPS_VALUE);
    meta->addEntry(OHOS_CONTROL_FPS_RANGES, fpsRange.data(), fpsRange.size());
    const int32_t DEVICE_STREAM_ID = 0;
    meta->addEntry(OHOS_CAMERA_STREAM_ID, &DEVICE_STREAM_ID, 1);
    std::vector<uint8_t> setting;
    MetadataUtils::ConvertMetadataToVec(meta, setting);
    display_->rc = (CamRetCode)display_->cameraDevice->UpdateSettings(setting);
    EXPECT_EQ(true, display_->rc == HDI::Camera::V1_0::NO_ERROR);

    // get preview
    display_->StartCapture(display_->STREAM_ID_PREVIEW, display_->CAPTURE_ID_PREVIEW, false, true);
    display_->StartCapture(display_->STREAM_ID_VIDEO, display_->CAPTURE_ID_VIDEO, false, true);

    // release stream
    display_->captureIds = {display_->CAPTURE_ID_PREVIEW, display_->CAPTURE_ID_VIDEO};
    display_->streamIds = {display_->STREAM_ID_PREVIEW, display_->STREAM_ID_VIDEO};
    display_->StopStream(display_->captureIds, display_->streamIds);
}

/**
  * @tc.name: USB Camera
  * @tc.desc: Commit 3 streams together, Preview,Video and still_capture streams, isStreaming is true.
  * @tc.level: Level0
  * @tc.size: MediumTest
  * @tc.type: Function
  */
TEST_F(UtestUSBCameraTest, camera_usb_0029)
{
    // Get the device manager
    display_->OpenUsbCamera();
    if (!g_usbCameraExit) {
        GTEST_SKIP() << "No usb camera plugged in" << std::endl;
    }
    // Get the stream manager
    display_->AchieveStreamOperator();
    // start stream
    display_->intents = {PREVIEW, STILL_CAPTURE, VIDEO};
    display_->StartStream(display_->intents);
    // Get preview
    display_->StartCapture(display_->STREAM_ID_PREVIEW, display_->CAPTURE_ID_PREVIEW, false, true);
    display_->StartCapture(display_->STREAM_ID_CAPTURE, display_->CAPTURE_ID_CAPTURE, false, true);
    display_->StartCapture(display_->STREAM_ID_VIDEO, display_->CAPTURE_ID_VIDEO, false, true);
    // release stream
    display_->captureIds = {display_->CAPTURE_ID_PREVIEW, display_->CAPTURE_ID_CAPTURE, display_->CAPTURE_ID_VIDEO};
    display_->streamIds = {display_->STREAM_ID_PREVIEW, display_->STREAM_ID_CAPTURE, display_->STREAM_ID_VIDEO};
    display_->StopStream(display_->captureIds, display_->streamIds);
}

/**
  * @tc.name: USB Camera
  * @tc.desc: get value of OHOS_SENSOR_ORIENTATION
  * @tc.level: Level0
  * @tc.size: MediumTest
  * @tc.type: Function
  */
TEST_F(UtestUSBCameraTest, camera_usb_0030)
{
    if (!g_usbCameraExit) {
        GTEST_SKIP() << "No usb camera plugged in" << std::endl;
    }
    ability_ = display_->GetCameraAbility();
    EXPECT_NE(ability_, nullptr);
    common_metadata_header_t *data = ability_->get();
    camera_metadata_item_t entry;
    int ret = FindCameraMetadataItem(data, OHOS_SENSOR_ORIENTATION, &entry);
    EXPECT_EQ(ret, CAM_META_SUCCESS);
    std::cout << "OHOS_SENSOR_ORIENTATION value is " << static_cast<int>(entry.data.u8[0]) << std::endl;
    EXPECT_TRUE(entry.data.u8[0] == 0);
}

/**
  * @tc.name: USB Camera
  * @tc.desc: get value of OHOS_ABILITY_FOCAL_LENGTH
  * @tc.level: Level0
  * @tc.size: MediumTest
  * @tc.type: Function
  */
TEST_F(UtestUSBCameraTest, camera_usb_0031)
{
    if (!g_usbCameraExit) {
        GTEST_SKIP() << "No usb camera plugged in" << std::endl;
    }
    ability_ = display_->GetCameraAbility();
    EXPECT_NE(ability_, nullptr);
    common_metadata_header_t *data = ability_->get();
    camera_metadata_item_t entry;
    int ret = FindCameraMetadataItem(data, OHOS_ABILITY_FOCAL_LENGTH, &entry);
    if (ret == CAM_META_SUCCESS) {
        std::cout << "log OHOS_ABILITY_FOCAL_LENGTH: count is " << (int)entry.count << std::endl;
        std::cout << "log focal length value: " << entry.data.f[0] << std::endl;
    } else if (ret == CAM_META_ITEM_NOT_FOUND) {
        std::cout << "log OHOS_ABILITY_FOCAL_LENGTH is not support" << std::endl;
    }
}