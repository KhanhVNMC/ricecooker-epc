# **NỒI CƠM ĐIỆN ĐIỀU KHIỂN ĐA CHẾ ĐỘ**

Firmware điều khiển nồi cơm điện chạy trên STM32F103C8T6 Blue Pill với FreeRTOS

## CHỨC NĂNG

- 5 chương trình nấu tự động: **RICE (Cơm thường), FAST (Cơm rút ngắn chế độ), PORR (Cháo), SOFT (Ninh), FLAN (Bánh flan/Caramel)**
- Nấu thủ công theo người dùng đặt, hẹn giờ và giữ ấm (nấc ủ)
- Nồi tự phát hiện ra lúc nước sôi, lúc cạn nước để điều chỉnh các chương trình tự động
- Có bảo vệ quá nhiệt, lỗi NTC và watchdog chống treo (nồi tự khởi động lại nếu bị treo)
- Điều khiển và xem nhiệt độ từ máy tính qua USB với giao thức LRCMDCTL (low-rate command control) qua lớp transport ECC (external command control)

## CÁC FILE TRONG RELEASE

- `ricecooker.bin`: nạp cho board thật
- `ricecooker.stripped.elf` (bản proteus): chạy mô phỏng trên Proteus 8.17

## NẠP FIRMWARE TRÊN MẠCH THẬT

Nối ST-Link V2 với board blue pill rồi chạy:

```sh
STM32_Programmer_CLI -c port=SWD -w ricecooker.bin 0x08000000 -v -rst
```

Sau đó cắm USB vào máy tính, máy sẽ nhận thiết bị tên **Flower Saint Cooker** (lsusb)

(C) 2026 KhanhVNMC, trandatpt and tuld1806
