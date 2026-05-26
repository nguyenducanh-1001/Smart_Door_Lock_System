#include "stm32f401xe.h"
#include <stdint.h>
#include <string.h>

// ================= 1. MODULE TIMER =================
void Timer2_Init(void) {
    RCC->APB1ENR |= (1 << 0);
    TIM2->PSC = 16000 - 1;
    TIM2->ARR = 0xFFFFFFFF;
    TIM2->EGR |= (1 << 0);
    TIM2->SR &= ~(1 << 0);
    TIM2->CR1 |= (1 << 0);
}

void Delay_ms(uint32_t ms) {
    TIM2->CNT = 0;
    while (TIM2->CNT < ms);
}

//======= 2. MODULE SERVO (TIM3_CH3 - PB0) =======
void Servo_Init(void){
	RCC->AHB1ENR |= 1 << 1;
	RCC->APB1ENR |= 1 << 1;
	GPIOB->MODER |= 0x2 << 0;
	GPIOB->AFR[0] |= 0x2 << 0; // AF2 - TIM3
	GPIOB->OSPEEDR |= 0x3 << 0;
	TIM3->PSC = 16 - 1;
	TIM3->ARR = 20000-1; // 20ms
	TIM3->CCR3 = 1100; // 1.1ms

	// PWM mode 1
	TIM3->CCMR2 |= (0x6 << 4) | (1 << 3);
	TIM3->CCER  |= (1 << 8);
	TIM3->EGR  |= (1 << 0);
    TIM3->CR1  |= (1 << 0);
}

void Servo_SetAngle(uint8_t angle) {
    if (angle > 180) angle = 180;
    TIM3->CCR3 = 1100 + ((uint32_t)angle * 2000 / 180);
}

// Mở cửa: quay tới 90° rồi dừng
void Servo_Open(void) {
    Servo_SetAngle(90);
}

// Đóng cửa: quay về 0°
void Servo_Close(void) {
    Servo_SetAngle(0);
}

// ================= 3. MODULE KEYPAD =================
char keys[4][4] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

void Keypad_Init(void) {
    RCC->AHB1ENR |= (1 << 0) | (1 << 2);
    GPIOA->MODER &= ~(0x003C000F);
    GPIOA->MODER |=  (0x00140005);
    GPIOC->MODER &= ~(0x000000FF);
    GPIOC->PUPDR &= ~(0x000000FF);
    GPIOC->PUPDR |=  (0x00000055);
}

char Keypad_Scan(void) {
    uint8_t row_pins[4] = {0, 1, 9, 10};
    for (int r = 0; r < 4; r++) {
        GPIOA->BSRR = (1 << 0) | (1 << 1) | (1 << 9) | (1 << 10);
        GPIOA->BSRR = (1 << (row_pins[r] + 16));
        Delay_ms(2);
        uint32_t port_c = GPIOC->IDR;
        for (int c = 0; c < 4; c++) {
            if ((port_c & (1 << c)) == 0) {
                Delay_ms(20);
                uint32_t timeout = 50000;
                while ((GPIOC->IDR & (1 << c)) == 0) {
                    if (--timeout == 0) break;
                }
                return keys[r][c];
            }
        }
    }
    return '\0';
}

// ================= 4. MODULE LCD & I2C =================
#define I2C_ADDR 0x4E

void I2C1_Init(void) {
    RCC->AHB1ENR |= (1 << 1);
    RCC->APB1ENR |= (1 << 21);
    GPIOB->MODER &= ~(0xF << 16);
    GPIOB->MODER |= (0xA << 16);
    GPIOB->OTYPER |= (3 << 8);
    GPIOB->PUPDR &= ~(0xF << 16);
    GPIOB->PUPDR |= (0x5 << 16);
    GPIOB->AFR[1] &= ~(0xFF);
    GPIOB->AFR[1] |= (0x44);
    I2C1->CR1 = (1 << 15); I2C1->CR1 = 0;
    I2C1->CR2 = 16;
    I2C1->CCR = 80;
    I2C1->TRISE = 17;
    I2C1->CR1 |= (1 << 0);
}

uint8_t I2C_WriteByte(uint8_t addr, uint8_t data) {
    uint32_t t;
    t = 10000;
    while (I2C1->SR2 & (1 << 1)) {
        if(!--t) return 1;
    }

    I2C1->CR1 |= (1 << 8);
    t = 10000;
    while (!(I2C1->SR1 & (1 << 0))) {
        if(!--t) return 1;
    }

    I2C1->DR = addr;
    t = 10000;
    while (!(I2C1->SR1 & (1 << 1))) {
        if (I2C1->SR1 & (1 << 10)) {
            I2C1->CR1 |= (1 << 9);
            return 1;
        }
        if (!--t) {
            I2C1->CR1 |= (1 << 9);
            return 1;
        }
    }

    (void)I2C1->SR1;
    (void)I2C1->SR2;
    t = 10000;
    while (!(I2C1->SR1 & (1 << 7))) {
        if(!--t) return 1;
    }

    I2C1->DR = data;
    t = 10000;
    while (!(I2C1->SR1 & (1 << 2))) {
        if(!--t) return 1;
    }
    I2C1->CR1 |= (1 << 9);
    return 0;
}

void LCD_Write_Nibble(uint8_t nibble) {
    I2C_WriteByte(I2C_ADDR, nibble | 0x0C); Delay_ms(1);
    I2C_WriteByte(I2C_ADDR, nibble | 0x08); Delay_ms(1);
}

void LCD_Send_Cmd(uint8_t cmd) {
    uint8_t data_u = (cmd & 0xF0);
    uint8_t data_l = ((cmd << 4) & 0xF0);
    I2C_WriteByte(I2C_ADDR, data_u | 0x0C);
    I2C_WriteByte(I2C_ADDR, data_u | 0x08);
    I2C_WriteByte(I2C_ADDR, data_l | 0x0C);
    I2C_WriteByte(I2C_ADDR, data_l | 0x08);
    Delay_ms(2);
}

void LCD_Send_Data(uint8_t data) {
    uint8_t data_u = (data & 0xF0);
    uint8_t data_l = ((data << 4) & 0xF0);
    I2C_WriteByte(I2C_ADDR, data_u | 0x0D);
    I2C_WriteByte(I2C_ADDR, data_u | 0x09);
    I2C_WriteByte(I2C_ADDR, data_l | 0x0D);
    I2C_WriteByte(I2C_ADDR, data_l | 0x09);
}

void LCD_Clear(void) { LCD_Send_Cmd(0x01); Delay_ms(2); }

void LCD_Set_Cursor(uint8_t row, uint8_t col) {
    uint8_t address = (row == 0) ? 0x80 : 0xC0;
    LCD_Send_Cmd(address + col);
}

void LCD_Init(void) {
    Delay_ms(50);
    LCD_Write_Nibble(0x30); Delay_ms(5);
    LCD_Write_Nibble(0x30); Delay_ms(1);
    LCD_Write_Nibble(0x30); Delay_ms(10);
    LCD_Write_Nibble(0x20); Delay_ms(10);
    LCD_Send_Cmd(0x28); Delay_ms(1);
    LCD_Send_Cmd(0x08); Delay_ms(1);
    LCD_Send_Cmd(0x01); Delay_ms(2);
    LCD_Send_Cmd(0x06); Delay_ms(1);
    LCD_Send_Cmd(0x0C); Delay_ms(1);
}

void LCD_Print(char* str) {
    while(*str) LCD_Send_Data(*str++);
}

void LCD_Print_String(const char *s) {
    while (*s)  LCD_Send_Data((uint8_t)*s++);
}

// ================= MODULE HO TRO =================
// Khởi tạo PC8 (LED Mở cửa) và PC7 (Còi báo động)
void Peripherals_Init(void) {
    RCC->AHB1ENR |= (1 << 2);
    // Cấu hình PC7 và PC8 làm Output (01)
    GPIOC->MODER &= ~((3 << 14) | (3 << 16));
    GPIOC->MODER |=  ((1 << 14) | (1 << 16));
}

void LED_On(void)     { GPIOC->BSRR = (1 << 8); }  // Bật PC8
void LED_Off(void)    { GPIOC->BSRR = (1 << 24); } // Tắt PC8

void Buzzer_On(void)  { GPIOC->BSRR = (1 << 7); }  // Bật PC7
void Buzzer_Off(void) { GPIOC->BSRR = (1 << 23); } // Tắt PC7


// ================= TÍNH NĂNG MỚI: BỘ NHỚ FLASH =================
#define FLASH_PASS_ADDR 0x08020000 // Địa chỉ Sector 5 (Rất an toàn)
#define PASS_LENGTH 6

void Flash_Unlock(void) {
    if (FLASH->CR & (1 << 31)) {
        FLASH->KEYR = 0x45670123;
        FLASH->KEYR = 0xCDEF89AB;
    }
}

void Flash_Lock(void) {
    FLASH->CR |= (1 << 31);
}

void Flash_Write_Password(char* pass) {
    Flash_Unlock();
    // Bước 1: Xóa trắng Sector 5
    while (FLASH->SR & (1 << 16));
    FLASH->CR &= ~(0xF << 3);
    FLASH->CR |= (5 << 3);
    FLASH->CR |= (1 << 1);
    FLASH->CR |= (1 << 16);
    while (FLASH->SR & (1 << 16));
    FLASH->CR &= ~(1 << 1);

    // Bước 2: Ghi mật khẩu mới vào
    FLASH->CR &= ~(3 << 8);
    FLASH->CR |= (1 << 0);
    for (int i = 0; i < PASS_LENGTH; i++) {
        *(volatile uint8_t*)(FLASH_PASS_ADDR + i) = pass[i];
        while (FLASH->SR & (1 << 16));
    }
    FLASH->CR &= ~(1 << 0);
    Flash_Lock();
}

// ================= 5. MODULE SMART LOCK =================
typedef enum {
    MODE_SETUP_INITIAL,
    MODE_NORMAL,
    MODE_OLD_PASS,
    MODE_NEW_PASS
} LockMode;

LockMode current_mode; // Sẽ khởi tạo ở hàm main khi check Flash

char master_pass[PASS_LENGTH];
char input_pass[PASS_LENGTH];
uint8_t pass_idx = 0;
// THÊM: Biến đếm số lần sai
uint8_t wrong_count = 0;

// Hàm so sánh chuỗi
uint8_t Check_Password(char* pass1, char* pass2) {
    for(int i = 0; i < PASS_LENGTH; i++) {
        if(pass1[i] != pass2[i]) return 0;
    }
    return 1;
}

// Hàm khởi tạo lại màn hình và xóa bộ đệm
void Lock_Reset_UI(char* line1) {
    pass_idx = 0; // Xóa sổ đếm
    LCD_Clear();
    LCD_Set_Cursor(0, 0); LCD_Print(line1);
    LCD_Set_Cursor(1, 0); // Đưa con trỏ xuống dòng 2 để chờ nhập
}

// Hàm xử lý Máy Trạng Thái FSM
void SmartLock_ProcessKey(char key) {

    // 1. NÚT '#' LÀ BACKSPACE
    if (key == '#') {
        if (pass_idx > 0) {
            pass_idx--;
            LCD_Set_Cursor(1, pass_idx);
            LCD_Send_Data(' ');
            LCD_Set_Cursor(1, pass_idx);
        }
        return;
    }

    // 2. NÚT '*' YÊU CẦU ĐỔI MẬT KHẨU
    if (key == '*') {
        if (current_mode == MODE_NORMAL && pass_idx == 0) {
            current_mode = MODE_OLD_PASS;
            Lock_Reset_UI("Xac nhan pass cu");
        }
        return;
    }

    // 3. NHẬP KÝ TỰ SỐ VÀ TỰ ĐỘNG XỬ LÝ KHI ĐỦ 6 SỐ
    if (key >= '0' && key <= '9') {
        if (pass_idx < PASS_LENGTH) {
            input_pass[pass_idx] = key;
            pass_idx++;
            LCD_Send_Data('*');
        }

        // CHỐT HẠ KHI VỪA GÕ ĐỦ 6 SỐ
        if (pass_idx == PASS_LENGTH) {

            // THIẾT LẬP MẬT KHẨU LẦN ĐẦU TIÊN
            if (current_mode == MODE_SETUP_INITIAL) {
                for(int i = 0; i < PASS_LENGTH; i++) {
                    master_pass[i] = input_pass[i];
                }

                // LƯU XUỐNG BỘ NHỚ FLASH
                Flash_Write_Password(master_pass);

                LCD_Clear();
                LCD_Set_Cursor(0, 0);
                LCD_Print("Da cai dat Pass!");
                Delay_ms(2000);

                current_mode = MODE_NORMAL;
                Lock_Reset_UI("Nhap mat khau:  ");
            }

            // NHẬP MẬT KHẨU ĐỂ MỞ CỬA
            else if (current_mode == MODE_NORMAL) {
                if (Check_Password(input_pass, master_pass)) {
                    // --- ĐÚNG PASS ---
                    wrong_count = 0; // Reset số lần sai

                    LCD_Clear();
                    LCD_Set_Cursor(0, 0); LCD_Print("  MO CUA...     ");
                    LED_On(); // Bật LED PC8 (Cửa đang mở)
                    Servo_Open();
                    Delay_ms(6000);

                    LED_Off(); // Tắt LED PC8 (Cửa đóng lại)
                    Servo_Close();
                    Lock_Reset_UI("Nhap mat khau:  ");
                } else {
                    // --- SAI PASS ---
                    wrong_count++; // Tăng biến đếm sai

                    LCD_Clear();
                    LCD_Set_Cursor(0, 0); LCD_Print("Sai mat khau!   ");

                    // Nếu sai 5 lần thì khóa 30 giây
                    if (wrong_count >= 5) {
                        LCD_Set_Cursor(1, 0);
                        LCD_Print("KHOA HE THONG!!!");
                        Buzzer_On();
                        Delay_ms(30000); // Đứng hình 30s còi réo liên tục
                        Buzzer_Off();
                        wrong_count = 0; // Hết giờ phạt thì reset lại
                    }
                    // Sai dưới 5 lần thì hú theo giây
                    else {
                        Buzzer_On();
                        Delay_ms(wrong_count * 1000); // VD: Sai lần 2 -> Hú 2s
                        Buzzer_Off();
                    }
                    Lock_Reset_UI("Nhap mat khau:  ");
                }
            }

            // XÁC MINH MẬT KHẨU CŨ
            else if (current_mode == MODE_OLD_PASS) {
                if (Check_Password(input_pass, master_pass)) {
                    current_mode = MODE_NEW_PASS;
                    Lock_Reset_UI("Nhap mat khauMoi");
                } else {
                    LCD_Clear();
                    LCD_Set_Cursor(0, 0); LCD_Print("Yeu cau tu choi!");

                    Buzzer_On();
                    Delay_ms(2000);
                    Buzzer_Off();

                    current_mode = MODE_NORMAL;
                    Lock_Reset_UI("Nhap mat khau:  ");
                }
            }

            // CẬP NHẬT MẬT KHẨU MỚI
            else if (current_mode == MODE_NEW_PASS) {
                for(int i = 0; i < PASS_LENGTH; i++) {
                    master_pass[i] = input_pass[i];
                }

                // LƯU XUỐNG BỘ NHỚ FLASH
                Flash_Write_Password(master_pass);

                LCD_Clear();
                LCD_Set_Cursor(0, 0);
                LCD_Print("Doi pass OK!    ");
                Delay_ms(2000);

                current_mode = MODE_NORMAL;
                Lock_Reset_UI("Nhap mat khau:  ");
            }
        }
    }
}


// ======= 6. MODULE RC522 và SPI1 =======

#define RC522_CS_LOW() GPIOB->BSRR = (1 << (12 + 16)) //PB12
#define RC522_CS_HIGH() GPIOB->BSRR = (1 << 12)
#define RC522_RST_LOW() GPIOB->BSRR = (1 << (2 + 16)) // PB2
#define RC522_RST_HIGH() GPIOB->BSRR = (1 << 2)

// Địa chỉ thanh ghi của RC522
#define CommandReg 0x01 // Thanh ghi lệnh
#define ComIEnReg 0x02 // Bật Interrupt
#define ComIrqReg 0x04 // Chứa cờ Interrupt
#define ErrorReg 0x06
#define FIFODataReg 0x09
#define FIFOLevelReg 0x0A
#define ControlReg 0x0C
#define BitFramingReg 0x0D
#define ModeReg 0x11
#define TxControlReg 0x14
#define TxASKReg 0x15
#define CRCResultRegH 0x21
#define CRCResultRegL 0x22
#define TModeReg 0x2A
#define TPrescalerReg 0x2B
#define TReloadRegH 0x2C
#define TReloadRegL 0x2D

// Định nghĩa mã lệnh RC522
// PCD: lệnh gửi cho RC522
// PICC: lệnh mà đầu đọc gửi qua sóng từ tới thẻ RFID
#define PCD_Idle 0x00 // Đưa RC522 về trạng thái nghỉ
#define PCD_CalcCRC 0x03 // RC522 tính CRC
#define PCD_Transceive 0x0C // Phát dữ liệu đến thẻ rồi nhận dữ liệu trả về
#define PCD_SoftReset 0x0F // Reset
#define PICC_REQA 0x26 // Phát hiện thẻ RFID chuẩn type A
#define PICC_AntiColl 0x93 // Chống va chạm khi đọc nhiều thẻ

void SPI1_Init(void) {
    RCC->AHB1ENR |= 1 << 1;
    // GPIOB
    RCC->APB2ENR |= 1 << 12;
    // SPI1
    GPIOB->MODER |=  (2 << 2*3) | (2 << 2*4) | (2 << 2*5);
    GPIOB->AFR[0] |=  (0x5 << 4*3) | (0x5 << 4*4) | (0x5 << 4*5);
    GPIOB->PUPDR |=  1 << 2*4;
    // MISO
    GPIOB->MODER |= 1 << 2*12;
    // PB12 CS
    GPIOB->MODER |= 1 << 2*2;
    // PB2 RST
    GPIOB->OSPEEDR |= (3 << 2*3) | (3 << 2*4) | (3 << 2*5) |
    (3 << 2*12) | (3 << 2*2);
    SPI1->CR1 = (1 << 2) | (0x3 << 3) |
    (1 << 8) | (1 << 9);
    SPI1->CR2 = 0x00;
    SPI1->CR1 |= (1 << 6);
    RC522_CS_HIGH();
    RC522_RST_HIGH();
}

uint8_t SPI1_Transfer(uint8_t data) {
    while (!(SPI1->SR & (1 << 1)));
    // Đợi TXE = 1, truyền data và0 DR để gửi đi
    SPI1->DR = data;
    while (!(SPI1->SR & (1 << 0))); // Đợi RXNE = 1, trả về dữ liệu trong DR
    return SPI1->DR;
}

void RC522_WriteReg(uint8_t reg, uint8_t val) {
    RC522_CS_LOW();
    // Theo chuẩn SPI của RC522, địa chỉ thanh ghi dịch trái 1 bit, bit MSB = 0 (ghi)
    // Địa chỉ max là 0x3F => max chỉ có 6 bit nhị phân
    SPI1_Transfer((reg << 1) & 0x7E);
    SPI1_Transfer(val);
    RC522_CS_HIGH();
}

uint8_t RC522_ReadReg(uint8_t reg) {
    RC522_CS_LOW();
    // Bit MSB = 1 (đọc)
    SPI1_Transfer(((reg << 1) & 0x7E) | 0x80);
    // Cho đi để nhận lại
    uint8_t val = SPI1_Transfer(0x00);
    RC522_CS_HIGH();
    return val;
}

void RC522_SetBitMask(uint8_t reg, uint8_t mask) {
    RC522_WriteReg(reg, RC522_ReadReg(reg) | mask);
}

void RC522_ClearBitMask(uint8_t reg, uint8_t mask) {
    RC522_WriteReg(reg, RC522_ReadReg(reg) & (~mask));
}

void RC522_Init(void) {
	// Reset để ổn định điện áp
    RC522_RST_LOW(); Delay_ms(10);
    RC522_RST_HIGH(); Delay_ms(50);
    // Reset mềm để đưa các thanh ghi về mặc định
    RC522_WriteReg(CommandReg, PCD_SoftReset);
    // Chờ cho đến khi bit PowerDown = 0, chip khởi động xong hoàn toàn
    uint32_t t = 100;
    do { Delay_ms(10); } while ((RC522_ReadReg(CommandReg) & (1 << 4)) && --t);
    // Cấu hình Timer cho RC522
    RC522_WriteReg(TModeReg, 0x8D);
    RC522_WriteReg(TPrescalerReg, 0x3E);
    RC522_WriteReg(TReloadRegH, 0x00);
    RC522_WriteReg(TReloadRegL, 0x1E);
    // Cấu hình modulation chuẩn ASK
    RC522_WriteReg(TxASKReg, 0x40);
    // CRC
    RC522_WriteReg(ModeReg, 0x3D);
    // cấp điện và bật anten
    RC522_SetBitMask(TxControlReg, 0x03);
    Delay_ms(10);
}

// Phát sóng tìm thẻ
uint8_t RC522_Request(void) {
    RC522_WriteReg(ComIEnReg, 0x77);
    RC522_WriteReg(ComIrqReg, 0x7F);
    RC522_SetBitMask(FIFOLevelReg, 0x80);
    RC522_WriteReg(CommandReg, PCD_Idle);
    RC522_WriteReg(BitFramingReg, 0x07);
    RC522_WriteReg(FIFODataReg, PICC_REQA);
    RC522_WriteReg(CommandReg, PCD_Transceive);
    RC522_SetBitMask(BitFramingReg, 0x80);
    // Chờ phản hồi từ thẻ
    uint32_t t = 2000;
    uint8_t irq;
    do {
        irq = RC522_ReadReg(ComIrqReg);
        if (!--t) return 0;
    } while (!(irq & 0x31));
    // Tắt bit truyền
    RC522_ClearBitMask(BitFramingReg, 0x80);
    // Kiểm tra lỗi
    if (RC522_ReadReg(ErrorReg) & 0x1B) return 0;
    if (!(irq & 0x30)) return 0;
    return 1;
}

// Đọc mã thẻ
uint8_t RC522_GetUID(uint8_t *uid) {
    RC522_WriteReg(ComIEnReg, 0x77);
    RC522_WriteReg(ComIrqReg, 0x7F);
    RC522_SetBitMask(FIFOLevelReg, 0x80);
    RC522_WriteReg(CommandReg, PCD_Idle);
    RC522_WriteReg(BitFramingReg, 0x00);
    RC522_WriteReg(FIFODataReg, PICC_AntiColl);
    RC522_WriteReg(FIFODataReg, 0x20);
    RC522_WriteReg(CommandReg, PCD_Transceive);
    RC522_SetBitMask(BitFramingReg, 0x80);

    uint32_t t = 2000;
    uint8_t irq;
    do {
        irq = RC522_ReadReg(ComIrqReg);
        if (!--t) return 0;
    } while (!(irq & 0x31));

    RC522_ClearBitMask(BitFramingReg, 0x80);
    if (RC522_ReadReg(ErrorReg) & 0x1B) return 0;
    if (RC522_ReadReg(FIFOLevelReg) < 4) return 0;
    for (uint8_t i = 0; i < 4; i++)
        uid[i] = RC522_ReadReg(FIFODataReg);
    return 1;
}

// ======= 7. RFID ACCESS CONTROL =======

// UID hợp lệ: F9:3F:4D:06
static const uint8_t authorized_uid[4] = {0xF9, 0x3F, 0x4D, 0x06};
// So sánh 4 byte UID, trả về 1 nếu khớp
uint8_t Compare_UID(const uint8_t *uid1, const uint8_t *uid2) {
    for (int i = 0; i < 4; i++)
        if (uid1[i] != uid2[i]) return 0;
    return 1;
}

void byte_to_hex(uint8_t b, char *out) {
    const char hex[] = "0123456789ABCDEF";
    out[0] = hex[(b >> 4) & 0x0F];
    out[1] = hex[b & 0x0F];
}

void Restore_Screen(void) {
    LCD_Clear();
    LCD_Set_Cursor(0, 0);
    switch (current_mode) {
        case MODE_SETUP_INITIAL: LCD_Print_String("Tao mat khau:   ");
        break;
        case MODE_NORMAL: LCD_Print_String("Nhap mat khau:  "); break;
        case MODE_OLD_PASS: LCD_Print_String("Xac nhan pass cu"); break;
        case MODE_NEW_PASS: LCD_Print_String("Nhap mat khauMoi"); break;
    }
    LCD_Set_Cursor(1, 0);
    for (int i = 0; i < pass_idx; i++) LCD_Send_Data('*');
}

/*
 * Đọc thẻ RFID:
 * - Thẻ hợp lệ (F9:3F:4D:06) => hiện "Mo cua!" 3s
 * - Thẻ không hợp lệ => hiện UID + "The sai!" + LED canh bao 2s
 * Trả về 1 nếu đã xử lý (để main loop bỏ qua keypad)
 */
uint8_t RFID_TryAccess(void) {
    uint8_t uid[4];
    if (!RC522_Request()) return 0;
    if (!RC522_GetUID(uid)) return 0;

    if (Compare_UID(uid, authorized_uid)) {
        // Thẻ đúng
        LCD_Clear();
        LCD_Set_Cursor(0, 0); LCD_Print_String("Mo cua!    ");
        LCD_Set_Cursor(1, 0); LCD_Print_String("Welcome Home :)");
        Servo_Open();
        LED_On();
        Buzzer_On();
        Delay_ms(500);
        Buzzer_Off();
        Delay_ms(5500);
        Servo_Close();
        LED_Off();
        Restore_Screen();
        Delay_ms(1000);
    } else {
        // Thẻ sai
        // Xây dựng chuỗi UID để hiển thị: "XX:XX:XX:XX"
        char uid_str[12];
        for (int i = 0; i < 4; i++) {
            byte_to_hex(uid[i], &uid_str[i * 3]);
            if (i < 3) uid_str[i * 3 + 2] = ':';
        }
        uid_str[11] = '\0';

        LCD_Clear();
        LCD_Set_Cursor(0, 0);
        LCD_Print_String("The sai!        ");
        for (int i = 0; i < 4; i++){
        	GPIOC->ODR ^= (1 << 8) | (1 << 7);
        	Delay_ms(1000);
        }

        Restore_Screen();
    }

    return 1;
}

// ================= 8. MAIN =================
int main(void) {
    Timer2_Init();
    Keypad_Init();
    I2C1_Init();
    LCD_Init();

    SPI1_Init();
    RC522_Init();
    Servo_Init();
    Delay_ms(1000);

    Peripherals_Init();
    LED_Off();
    // Mặc định cửa đóng
    Buzzer_Off(); // Mặc định còi tắt

    // === KIỂM TRA BỘ NHỚ FLASH KHI KHỞI ĐỘNG ===
    uint8_t first_byte = *(volatile uint8_t*)FLASH_PASS_ADDR;

    if (first_byte == 0xFF) {
        // 0xFF là trạng thái trống, chưa từng cài pass
        current_mode = MODE_SETUP_INITIAL;
        LCD_Clear();
        Lock_Reset_UI("Tao mat khau:   ");
    } else {
        // Đã có mật khẩu, load từ Flash vào RAM
        for (int i = 0; i < PASS_LENGTH; i++) {
            master_pass[i] = *(volatile uint8_t*)(FLASH_PASS_ADDR + i);
        }
        current_mode = MODE_NORMAL;
        LCD_Clear();
        Lock_Reset_UI("Nhap mat khau:  ");
    }
    // ==========================================

    char key;
    char last_key = '\0';
    while (1) {
        // Chỉ quét RFID khi không có phím đang giữ
        if (last_key == '\0') {
            if (RFID_TryAccess()) {
                // Cooldown
                Delay_ms(500);
                while (RC522_Request()) {
                    Delay_ms(200);
                }
                Delay_ms(300);
            }
        }

        key = Keypad_Scan();
        // Nếu có phím được nhấn (chống dội đè phím)
        if (key != '\0' && key != last_key) {

            // Giao mọi logic cho module FSM xử lý
            SmartLock_ProcessKey(key);
            last_key = key;     // Lưu lại trạng thái
        }

        // Nhả phím ra
        if (key == '\0') {
            last_key = '\0';
        }
    }
}
