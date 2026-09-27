#include "usbd_cdc.h"
#include "include/ecclayer.h"
#include "include/xthreads.h"

static USBD_HandleTypeDef usb_device; // generic USB protocol stack state
static PCD_HandleTypeDef usb_pcd; // actual STM32 USB peripheral state

static uint8_t rx_buffer[CDC_DATA_FS_MAX_PACKET_SIZE]; // USB FS bulk packet = max 64 bytes
static uint8_t tx_buffer[ECC_MAX_DATAGRAM_SIZE]; // TX staging buf
static xthread_handle_t *tx_caller_thread;

// USB device description bullshit
static uint8_t usb_string_buf[USBD_MAX_STR_DESC_SIZ];
static uint8_t usb_device_desc[USB_LEN_DEV_DESC] = {
    0x12, USB_DESC_TYPE_DEVICE,
    0x00, 0x02, // usb 2.0
    0x02, 0x00, 0x00,
    USB_MAX_EP0_SIZE,
    0x36, 0x18, // Vendor ID = 1836
    0x36, 0x36, // Product ID = 3636
    0x00, 0x01,
    USBD_IDX_MFC_STR, USBD_IDX_PRODUCT_STR, 0, // desc indx
    1
};
static uint8_t usb_lang_id_desc[4] = {
    0x04, // bLength (4 bytes)
    USB_DESC_TYPE_STRING, // bDescriptorType (0x03)
    0x09, 0x40 // english
};

/////////// THIS IS GENERIC STM32 USB CODE, IM NOT GONNA EVEN PRETEND TO UNDERSTAND ALL OF  IT ///////////
/// i just stole this code from stm32cubeide ig
/////////////////////////////////////////////////////////////////////////////////////////////////////////
USBD_StatusTypeDef USBD_LL_Init(USBD_HandleTypeDef *pdev) {
    usb_pcd.Instance = USB;
    usb_pcd.Init.dev_endpoints = 8;
    usb_pcd.Init.speed = PCD_SPEED_FULL;
    usb_pcd.Init.ep0_mps = PCD_EP0MPS_64;
    usb_pcd.Init.phy_itface = PCD_PHY_EMBEDDED;
    usb_pcd.Init.Sof_enable = DISABLE;
    usb_pcd.Init.low_power_enable = DISABLE;
    usb_pcd.Init.lpm_enable = DISABLE;
    usb_pcd.Init.battery_charging_enable = DISABLE;  // this ain't a charger
    usb_pcd.pData = pdev;
    pdev->pData = &usb_pcd;
    if (HAL_PCD_Init(&usb_pcd) != HAL_OK) { // initialize actual USB hardware
        return USBD_FAIL;
    }
    // i have no phucking idea what the fuck this is
    HAL_PCDEx_PMAConfig(&usb_pcd, 0x00, PCD_SNG_BUF, 0x018); // EP0 OUT
    HAL_PCDEx_PMAConfig(&usb_pcd, 0x80, PCD_SNG_BUF, 0x058); // EP0 IN
    HAL_PCDEx_PMAConfig(&usb_pcd, CDC_IN_EP, PCD_SNG_BUF, 0x0C0); // CDC -> PC
    HAL_PCDEx_PMAConfig(&usb_pcd, CDC_CMD_EP,PCD_SNG_BUF, 0x100); // CDC notification EP
    HAL_PCDEx_PMAConfig(&usb_pcd, CDC_OUT_EP, PCD_SNG_BUF, 0x110); // PC -> CDC
    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_DeInit(USBD_HandleTypeDef *pdev) {
    return HAL_PCD_DeInit(pdev->pData) == HAL_OK ? USBD_OK : USBD_FAIL;
}

USBD_StatusTypeDef USBD_LL_Start(USBD_HandleTypeDef *pdev) {
    return HAL_PCD_Start(pdev->pData) == HAL_OK ? USBD_OK : USBD_FAIL;
}

USBD_StatusTypeDef USBD_LL_Stop(USBD_HandleTypeDef *pdev) {
    return HAL_PCD_Stop(pdev->pData) == HAL_OK ? USBD_OK : USBD_FAIL;
}

USBD_StatusTypeDef USBD_LL_OpenEP(USBD_HandleTypeDef *pdev, const uint8_t ep_addr, const uint8_t ep_type, const uint16_t ep_mps) {
    return HAL_PCD_EP_Open(pdev->pData, ep_addr, ep_mps, ep_type) == HAL_OK ? USBD_OK : USBD_FAIL;
}

USBD_StatusTypeDef USBD_LL_CloseEP(USBD_HandleTypeDef *pdev, const uint8_t ep_addr) {
    return HAL_PCD_EP_Close(pdev->pData, ep_addr) == HAL_OK ? USBD_OK : USBD_FAIL;
}

USBD_StatusTypeDef USBD_LL_FlushEP(USBD_HandleTypeDef *pdev,const uint8_t ep_addr) {
    return HAL_PCD_EP_Flush(pdev->pData, ep_addr) == HAL_OK ? USBD_OK : USBD_FAIL;
}

USBD_StatusTypeDef USBD_LL_StallEP(USBD_HandleTypeDef *pdev, const uint8_t ep_addr) {
    return HAL_PCD_EP_SetStall(pdev->pData, ep_addr) == HAL_OK ? USBD_OK : USBD_FAIL;
}

USBD_StatusTypeDef USBD_LL_ClearStallEP(USBD_HandleTypeDef *pdev, const uint8_t ep_addr) {
    return HAL_PCD_EP_ClrStall(pdev->pData, ep_addr) == HAL_OK ? USBD_OK : USBD_FAIL;
}

uint8_t USBD_LL_IsStallEP(USBD_HandleTypeDef *pdev, const uint8_t ep_addr) {
    const PCD_HandleTypeDef *pcd = pdev->pData;
    const uint8_t ep = ep_addr & 0x7FU;
    return (ep_addr & 0x80U) ? pcd->IN_ep[ep].is_stall : pcd->OUT_ep[ep].is_stall;
}

USBD_StatusTypeDef USBD_LL_SetUSBAddress(USBD_HandleTypeDef *pdev, const uint8_t dev_addr) {
    return HAL_PCD_SetAddress(pdev->pData, dev_addr) == HAL_OK ? USBD_OK : USBD_FAIL;
}

USBD_StatusTypeDef USBD_LL_Transmit(USBD_HandleTypeDef *pdev, const uint8_t ep_addr, uint8_t *pbuf, const uint32_t size) {
    return HAL_PCD_EP_Transmit(pdev->pData, ep_addr, pbuf, size) == HAL_OK ? USBD_OK : USBD_FAIL;
}

USBD_StatusTypeDef USBD_LL_PrepareReceive(USBD_HandleTypeDef *pdev, const uint8_t ep_addr, uint8_t *pbuf, const uint32_t size) {
    return HAL_PCD_EP_Receive(pdev->pData, ep_addr, pbuf, size) == HAL_OK ? USBD_OK : USBD_FAIL;
}

uint32_t USBD_LL_GetRxDataSize(USBD_HandleTypeDef *pdev, const uint8_t ep_addr) {
    return HAL_PCD_EP_GetRxCount(pdev->pData, ep_addr); // ask how many bytes arrived
}

void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd) {
    USBD_LL_SetupStage(hpcd->pData, (uint8_t*)hpcd->Setup);
}

void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, const uint8_t epnum) {
    USBD_LL_DataOutStage(hpcd->pData, epnum, hpcd->OUT_ep[epnum].xfer_buff);
}

void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, const uint8_t epnum) {
    USBD_LL_DataInStage(hpcd->pData, epnum, hpcd->IN_ep[epnum].xfer_buff);
}

void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd) {
    USBD_LL_SetSpeed(hpcd->pData, USBD_SPEED_FULL);
    USBD_LL_Reset(hpcd->pData);
}

void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *hpcd) {
    USBD_LL_Suspend(hpcd->pData);
}

void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *hpcd) {
    USBD_LL_Resume(hpcd->pData);
}

void HAL_PCD_MspInit(PCD_HandleTypeDef *hpcd) {
    if (hpcd->Instance != USB) {
        return;
    }
    __HAL_RCC_USB_CLK_ENABLE();
    // configure USB interrupt priority
    HAL_NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn,5, 0);
    HAL_NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn); // allow USB to interrupt CPU
}

void USB_LP_CAN1_RX0_IRQHandler(void) {
    HAL_PCD_IRQHandler(&usb_pcd); // hand raw USB interrupt to HAL
}
///////////////////// END OF STOLEN CODE /////////////////////

#ifdef COMPILED_FOR_PROTEUS_8_17
__attribute__((optimize(0)))
#endif
bool ecc_is_tx_ready(void) {
    const USBD_CDC_HandleTypeDef *cdc = (USBD_CDC_HandleTypeDef*)usb_device.pClassData;
    // TxState == 0 -> IDLE, ready to tx
    return cdc != nullptr && cdc->TxState == 0U;
}

#ifdef COMPILED_FOR_PROTEUS_8_17
__attribute__((optimize(0)))
#endif
static bool ecc_data_tx_generic(const void *data, const uint16_t length, const bool wait_until_ready) {
    // reject if usb is not ready or invalid length
    if (!ecc_is_layer_ready() || length > sizeof(tx_buffer)) {
        return false;
    }
    if (!ecc_is_tx_ready()) { // previous TX still active
        if (!wait_until_ready) {
            return false; // no wait, fail immediately
        }

        // publish the caller thread so that cdc_tx_complete can notify it once
        tx_caller_thread = xthreads_current();
        if (!ecc_is_tx_ready()) {
            xthreads_bwait(XTHREAD_WAIT_ALL, 10); // deadline 10ms
        }
        tx_caller_thread = nullptr;

        // sanity check
        if (!ecc_is_tx_ready()) {
            // timed out & not ready yet
            // OR woke up for the wrong reason & not ready yet
            return false;
        }
        // somehow we're ready now, not complaining!
    }
    // copy data to the staging buf
    memcpy(tx_buffer, data, length);
    USBD_CDC_SetTxBuffer(&usb_device, tx_buffer, length);
    return USBD_CDC_TransmitPacket(&usb_device) == USBD_OK;
}

bool ecc_tx_data_async(const void *data, const uint16_t length) {
    return ecc_data_tx_generic(data, length, false);
}

bool ecc_tx_data(const void *data, const uint16_t length) {
    return ecc_data_tx_generic(data, length, true);
}

static int8_t cdc_init(void) {
    USBD_CDC_SetTxBuffer(&usb_device, tx_buffer, 0); // tell CDC where tx data will live
    ecc_next_read_ready();
    return USBD_OK;
}

static int8_t cdc_deinit(void) {
    return USBD_OK;
}

static int8_t cdc_control(uint8_t, uint8_t*, uint16_t) {
    return USBD_OK;
}

static int8_t cdc_tx_complete(uint8_t*, uint32_t*, uint8_t) {
    xthreads_notify(tx_caller_thread);
    return USBD_OK;
}

static int8_t cdc_receive(uint8_t *buffer, uint32_t *length_ptr) {
    switch (ecc_on_rx_data(buffer, *length_ptr)) {
        case ECC_RX_PARTIAL:
        case ECC_RX_REJECTED: {
            ecc_next_read_ready(); // allow reading next usb packet instantly
            break;
        }
        case ECC_RX_COMPLETE: {
            break; // latch until the consumer releases
        }
    }
    return USBD_OK;
}

// this function will make the usb not NAK inbound packets anymore
// and actually receive it and put it in rx buf
void ecc_next_read_ready(void) {
    USBD_CDC_SetRxBuffer(&usb_device, rx_buffer); // tell CDC where inc rx go
    USBD_CDC_ReceivePacket(&usb_device); // arm the OUT endpoint
}

static USBD_CDC_ItfTypeDef cdc_callback_funcs = {
    .Init = cdc_init,
    .DeInit = cdc_deinit,
    .Control = cdc_control,
    .Receive = cdc_receive,
    .TransmitCplt = cdc_tx_complete,
};

// this configures what the usb report as
static uint8_t* usb_get_device_descriptor(const USBD_SpeedTypeDef, uint16_t *length) {
    *length = sizeof usb_device_desc;
    return usb_device_desc;
}

static uint8_t* usb_get_lang_id_descriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
    (void)speed;
    *length = sizeof(usb_lang_id_desc);
    return usb_lang_id_desc;
}

static uint8_t* usb_get_manufacturer_string(USBD_SpeedTypeDef, uint16_t *length) {
    USBD_GetString((uint8_t*) "Nha may Z111", usb_string_buf, length);
    return usb_string_buf;
}

static uint8_t* usb_get_product_string(USBD_SpeedTypeDef, uint16_t *length) {
    // flower saint -> hoa thánh -> thanh hóa
    USBD_GetString((uint8_t*) "Flower Saint Cooker", usb_string_buf, length);
    return usb_string_buf;
}

static USBD_DescriptorsTypeDef usb_descriptors = {
    .GetDeviceDescriptor = usb_get_device_descriptor,
    .GetLangIDStrDescriptor = usb_get_lang_id_descriptor,
    .GetManufacturerStrDescriptor = usb_get_manufacturer_string,
    .GetProductStrDescriptor = usb_get_product_string,
};

static bool usb_initialized = false;

bool ecc_is_layer_ready() {
    return usb_initialized && usb_device.dev_state == USBD_STATE_CONFIGURED;
}

#ifdef COMPILED_FOR_PROTEUS_8_17
__attribute__((optimize(0)))
#endif
bool ecc_init_stack(void) {
    usb_initialized = false;
    // initialize generic USB protocol machinery
    if (USBD_Init(&usb_device, &usb_descriptors, 0U) != USBD_OK) {
        return false;
    }
    // make this USB device a CDC device (emulating a COM port)
    if (USBD_RegisterClass(&usb_device, &USBD_CDC) != USBD_OK) {
        return false;
    }
    if (USBD_CDC_RegisterInterface(&usb_device, &cdc_callback_funcs) != USBD_OK) {
        return false;
    }
    return usb_initialized = (USBD_Start(&usb_device) == USBD_OK); // start hardware and begin enumeration
}

// DUMMY static malloc and free, it just store in static memory
// this is NOT a real heap allocator, its just cdc
static USBD_CDC_HandleTypeDef cdc_state;
void* usbd_static_mem_dmalloc(const uint32_t size) {
    if (size > sizeof(cdc_state)) {
        return nullptr;
    }
    return &cdc_state;
}
void usbd_static_mem_dfree(void*) {}