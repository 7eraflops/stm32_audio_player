/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"
#include "usb_host.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "CS43L22.h"
#include "audio_i2s.h"
#include "files.h"
#include "lcd16x2_i2c.h"
#include "player.h"

#include "Flac.hpp"
#include "Wav.hpp"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define NUM_ROWS 4
#define NUM_COLS 4

#define SCROLL_DELAY 250

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c3;

I2S_HandleTypeDef hi2s3;
DMA_HandleTypeDef hdma_spi3_tx;

RNG_HandleTypeDef hrng;

TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
// random number
uint32_t random_number;
bool random_number_ready = false;

// files
extern ApplicationTypeDef Appli_state;
bool is_usb_mounted = false;
extern FILE_LIST wav_file_list;
extern FILE_LIST flac_file_list;
FILE_LIST *current_file_list = &wav_file_list;

extern FIL flac_file;
extern FIL wav_file;

// playback
bool continue_playback = false;
bool play_pause_toggle = false;
bool shuffle_toggle = false;
bool loop_toggle = false;

// volume
bool mute_toggle = false;
uint8_t volume = 191;
uint8_t chars_to_display;

// text
char scroll_text[MAX_FILENAME_LENGTH] __attribute__((section(".ccmram")));
uint8_t text_length = 0;
volatile uint8_t current_position = 0;
bool scrolling_active = false;

// buttons
volatile uint8_t key_number = 0;

GPIO_InitTypeDef GPIO_InitStructPrivate = {0};

GPIO_TypeDef *ROW_PORTS[4] = {
    KEYBOARD_R1_GPIO_Port,
    KEYBOARD_R2_GPIO_Port,
    KEYBOARD_R3_GPIO_Port,
    KEYBOARD_R4_GPIO_Port};

constexpr uint16_t ROW_PINS[4] = {
    KEYBOARD_R1_Pin,
    KEYBOARD_R2_Pin,
    KEYBOARD_R3_Pin,
    KEYBOARD_R4_Pin};

constexpr uint16_t COL_PINS[4] = {
    KEYBOARD_C1_Pin,
    KEYBOARD_C2_Pin,
    KEYBOARD_C3_Pin,
    KEYBOARD_C4_Pin};

// Lookup table for key values
constexpr uint8_t KEY_VALUES[4][4] = {
    {1, 2, 3, 4},    // Row 1 values
    {5, 6, 7, 8},    // Row 2 values
    {9, 10, 11, 12}, // Row 3 values
    {13, 14, 15, 16} // Row 4 values
};

// debouncing
volatile uint32_t previous_time = 0;
volatile uint32_t current_time = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2S3_Init(void);
static void MX_I2C3_Init(void);
static void MX_TIM2_Init(void);
static void MX_RNG_Init(void);
void MX_USB_HOST_Process(void);

/* USER CODE BEGIN PFP */
static void initialize_peripherals(void);
static void handle_application_state(void);
static void file_select(void);
static void decode_file(void);
static void handle_player_controls(void);
static void handle_play_pause_toggle(void);
static void update_track_display(void);
static void display_long_text(const char *text);
static void update_volume_bar(uint8_t volume);
static void handle_usb_mounting(void);
static void handle_playback(void);
static void advance_to_next_track(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void initialize_peripherals()
{
    // RNG init
    HAL_RNG_GenerateRandomNumber_IT(&hrng);

    // Audio init
    CS43_Init(hi2c1, CS43_MODE_I2S);
    CS43_set_volume(volume);
    CS43_enable_right_left(CS43_RIGHT_LEFT);
    audio_i2s_set_handle(&hi2s3);

    // LCD init
    lcd16x2_i2c_init(&hi2c3);
    lcd16x2_i2c_load_audio_characters();
    HAL_TIM_Base_Start_IT(&htim2);

    // Red LED init
    HAL_GPIO_WritePin(LD5_RED_GPIO_Port, LD5_RED_Pin, GPIO_PIN_SET);
}

static void handle_application_state(void)
{
    if (Appli_state == APPLICATION_START)
    {
        HAL_GPIO_WritePin(LD4_GREEN_GPIO_Port, LD4_GREEN_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LD5_RED_GPIO_Port, LD5_RED_Pin, GPIO_PIN_RESET);
    }
    else if (Appli_state == APPLICATION_DISCONNECT)
    {
        HAL_GPIO_WritePin(LD4_GREEN_GPIO_Port, LD4_GREEN_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LD5_RED_GPIO_Port, LD5_RED_Pin, GPIO_PIN_SET);
    }
}

static void handle_usb_mounting(void)
{
    if (!is_usb_mounted)
    {
        f_mount(&USBHFatFS, (const TCHAR *)USBHPath, 1);
        is_usb_mounted = true;
        scan_usb("wav");
        lcd16x2_i2c_set_cursor(1, 0);
        lcd16x2_i2c_printf("WAV Select");
    }
}

static void handle_playback(void)
{
    update_track_display();

    if (!continue_playback)
    {
        file_select();
    }

    lcd16x2_i2c_set_cursor(1, 8);
    lcd16x2_i2c_print_audio_character("//p");
    update_volume_bar(volume);

    player_file_select(wav_file_list.filenames[wav_file_list.current_index]);
    player_play();

    while (!player_is_finished() && continue_playback)
    {
        player_process();
        handle_player_controls();
    }

    if (continue_playback)
    {
        audio_i2s_stop();
        advance_to_next_track();
        update_track_display();
    }
}

static void update_track_display(void)
{
    lcd16x2_i2c_set_cursor(0, 0);
    lcd16x2_i2c_printf("                ");
    lcd16x2_i2c_set_cursor(0, 0);
    display_long_text(current_file_list->filenames[current_file_list->current_index]);
}

static void file_select(void)
{
    while (!continue_playback)
    {
        switch (key_number)
        {
        case 5: // Previous file
            current_file_list->current_index = (current_file_list->current_index - 1 + current_file_list->count) % current_file_list->count;
            update_track_display();
            key_number = 0;
            break;
        case 6: // Select
            if (current_file_list == &wav_file_list)
            {
                continue_playback = true;
            }
            else if (current_file_list == &flac_file_list)
            {
                decode_file();
            }
            key_number = 0;
            break;
        case 7: // Next file
            current_file_list->current_index = (current_file_list->current_index + 1) % current_file_list->count;
            update_track_display();
            key_number = 0;
            break;
        case 16: // Wav/Flac mode toggle
            lcd16x2_i2c_clear();
            if (current_file_list == &wav_file_list)
            {
                current_file_list = &flac_file_list;
                scan_usb("flac");
                lcd16x2_i2c_set_cursor(1, 0);
                lcd16x2_i2c_printf("FLAC Select");
            }
            else
            {
                current_file_list = &wav_file_list;
                scan_usb("wav");
                lcd16x2_i2c_set_cursor(1, 0);
                lcd16x2_i2c_printf("WAV Select");
            }
            update_track_display();
            key_number = 0;
            break;
        }
        if (continue_playback)
        {
            break;
        }
    }
}

static void update_volume_bar(uint8_t volume)
{
    chars_to_display = (volume * 16 + 127) / 255;

    lcd16x2_i2c_set_cursor(1, 0);
    for (uint8_t i = 0; i < 16; i++)
    {
        lcd16x2_i2c_print_audio_character(i < chars_to_display ? "//v" : " ");
    }

    lcd16x2_i2c_set_cursor(1, 8);
    if (chars_to_display > 8)
    {
        lcd16x2_i2c_print_audio_character(play_pause_toggle ? "//I" : "//i");
    }
    else
    {
        lcd16x2_i2c_print_audio_character(play_pause_toggle ? "//P" : "//p");
    }
}

static void decode_file(void)
{
    char temp_filename[256]{};
    char final_filename[256]{};
    f_close(&flac_file);
    f_close(&wav_file);

    if (f_open(&flac_file, flac_file_list.filenames[flac_file_list.current_index], FA_READ) != FR_OK)
    {
        return;
    }

    strcpy(temp_filename, flac_file_list.filenames[flac_file_list.current_index]);
    strcpy(final_filename, flac_file_list.filenames[flac_file_list.current_index]);
    char *temp_ext = strstr(temp_filename, ".flac");
    char *final_ext = strstr(final_filename, ".flac");
    if (temp_ext && final_ext)
    {
        strcpy(temp_ext, ".temp");
        strcpy(final_ext, ".wav");
    }

    if (f_open(&wav_file, temp_filename, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
    {
        f_close(&flac_file);
        return;
    }

    Flac decoder(flac_file);
    decoder.initialize();

    Wav writer(wav_file, decoder.get_stream_info());

    uint64_t samples_per_box = decoder.get_stream_info().total_samples / 16;
    uint8_t current_boxes = 0;

    lcd16x2_i2c_set_cursor(1, 0);
    lcd16x2_i2c_printf("                ");
    lcd16x2_i2c_set_cursor(1, 0);

    while (!decoder.get_reader().eos())
    {
        decoder.decode_frame();
        writer.write_samples(decoder.get_audio_buffer());

        uint8_t boxes_should_show = decoder.get_sample_count() / samples_per_box;

        if (boxes_should_show > current_boxes && current_boxes < 16)
        {
            lcd16x2_i2c_print_audio_character("//v");
            current_boxes++;
        }

        if (key_number == 1)
        {
            f_close(&flac_file);
            f_close(&wav_file);
            f_unlink(temp_filename);
            update_track_display();
            lcd16x2_i2c_set_cursor(1, 0);
            lcd16x2_i2c_printf("FLAC Select     ");
            key_number = 0;
            return;
        }
    }

    f_close(&flac_file);
    f_close(&wav_file);
    f_rename(temp_filename, final_filename);
    f_unlink(temp_filename);

    update_track_display();
    lcd16x2_i2c_set_cursor(1, 0);
    lcd16x2_i2c_printf("FLAC Select     ");

    return;
}

static void handle_player_controls(void)
{
    switch (key_number)
    {
    case 1: // Stop
        lcd16x2_i2c_clear();
        player_stop();
        continue_playback = false;
        play_pause_toggle = 0;
        lcd16x2_i2c_set_cursor(1, 0);
        lcd16x2_i2c_printf("WAV Select");
        break;
    case 2: // Volume Up
        volume = (255 - volume >= 16) ? volume + 16 : 255;
        CS43_set_volume(volume);
        update_volume_bar(volume);
        break;
    case 3: // Mute
        mute_toggle ^= 1;
        CS43_set_mute(mute_toggle);
        update_volume_bar(mute_toggle ? 0 : volume);
        break;
    case 5: // Previous Track
        shuffle_toggle ? player_random_track() : player_previous_track();
        player_play();
        lcd16x2_i2c_set_cursor(1, 8);
        lcd16x2_i2c_print_audio_character(chars_to_display > 8 ? "//i" : "//p");
        update_track_display();
        break;
    case 6: // Play/Pause
        handle_play_pause_toggle();
        break;
    case 7: // Next Track
        shuffle_toggle ? player_random_track() : player_next_track();
        player_play();
        lcd16x2_i2c_set_cursor(1, 8);
        lcd16x2_i2c_print_audio_character(chars_to_display > 8 ? "//i" : "//p");
        update_track_display();
        break;
    case 9: // Shuffle mode
        shuffle_toggle ^= 1;
        loop_toggle = 0;
        HAL_GPIO_TogglePin(LD3_ORANGE_GPIO_Port, LD3_ORANGE_Pin);
        HAL_GPIO_WritePin(LD6_BLUE_GPIO_Port, LD6_BLUE_Pin, GPIO_PIN_RESET);
        break;
    case 10: // Volume Down
        volume = (volume >= 16) ? volume - 16 : 0;
        CS43_set_volume(volume);
        update_volume_bar(volume);
        break;
    case 11: // Loop mode
        loop_toggle ^= 1;
        shuffle_toggle = 0;
        HAL_GPIO_TogglePin(LD6_BLUE_GPIO_Port, LD6_BLUE_Pin);
        HAL_GPIO_WritePin(LD3_ORANGE_GPIO_Port, LD3_ORANGE_Pin, GPIO_PIN_RESET);
        break;
    default:
        break;
    }
    key_number = 0;
}

static void handle_play_pause_toggle(void)
{
    play_pause_toggle ^= 1;
    lcd16x2_i2c_set_cursor(1, 8);
    if (play_pause_toggle)
    {
        player_pause();
        lcd16x2_i2c_print_audio_character(chars_to_display > 8 ? "//I" : "//P");
    }
    else
    {
        player_resume();
        lcd16x2_i2c_print_audio_character(chars_to_display > 8 ? "//i" : "//p");
    }
}

static void display_long_text(const char *text)
{
    snprintf(scroll_text, MAX_FILENAME_LENGTH - 1, "%s%s", text, strlen(text) > 16 ? "      " : "");
    scroll_text[MAX_FILENAME_LENGTH - 1] = '\0';
    text_length = strlen(scroll_text);

    char display[17] = {0};
    strncpy(display, scroll_text, 16);

    lcd16x2_i2c_printf(display);

    scrolling_active = (text_length > 16);
    current_position = 0;
}

static void advance_to_next_track(void)
{
    if (shuffle_toggle)
    {
        uint32_t new_index;
        do
        {
            if (random_number_ready)
            {
                random_number = HAL_RNG_ReadLastRandomNumber(&hrng);
                random_number_ready = false;
                HAL_RNG_GenerateRandomNumber_IT(&hrng);
            }
            new_index = random_number % wav_file_list.count;
        } while (new_index == wav_file_list.current_index);

        wav_file_list.current_index = new_index;
    }
    else if (!loop_toggle)
    {
        wav_file_list.current_index = (wav_file_list.current_index + 1) % wav_file_list.count;
    }
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{

    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */

    /* MCU Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_I2C1_Init();
    MX_I2S3_Init();
    MX_I2C3_Init();
    MX_USB_HOST_Init();
    MX_FATFS_Init();
    MX_TIM2_Init();
    MX_RNG_Init();
    /* USER CODE BEGIN 2 */
    initialize_peripherals();

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        /* USER CODE END WHILE */
        MX_USB_HOST_Process();
        handle_application_state();

        /* USER CODE BEGIN 3 */
        if (Appli_state == APPLICATION_READY)
        {
            handle_usb_mounting();
            handle_playback();
        }
    }

    /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Configure the main internal regulator output voltage
     */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief I2C1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C1_Init(void)
{

    /* USER CODE BEGIN I2C1_Init 0 */

    /* USER CODE END I2C1_Init 0 */

    /* USER CODE BEGIN I2C1_Init 1 */

    /* USER CODE END I2C1_Init 1 */
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN I2C1_Init 2 */

    /* USER CODE END I2C1_Init 2 */
}

/**
 * @brief I2C3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C3_Init(void)
{

    /* USER CODE BEGIN I2C3_Init 0 */

    /* USER CODE END I2C3_Init 0 */

    /* USER CODE BEGIN I2C3_Init 1 */

    /* USER CODE END I2C3_Init 1 */
    hi2c3.Instance = I2C3;
    hi2c3.Init.ClockSpeed = 100000;
    hi2c3.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c3.Init.OwnAddress1 = 0;
    hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c3.Init.OwnAddress2 = 0;
    hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c3) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN I2C3_Init 2 */

    /* USER CODE END I2C3_Init 2 */
}

/**
 * @brief I2S3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2S3_Init(void)
{

    /* USER CODE BEGIN I2S3_Init 0 */

    /* USER CODE END I2S3_Init 0 */

    /* USER CODE BEGIN I2S3_Init 1 */

    /* USER CODE END I2S3_Init 1 */
    hi2s3.Instance = SPI3;
    hi2s3.Init.Mode = I2S_MODE_MASTER_TX;
    hi2s3.Init.Standard = I2S_STANDARD_PHILIPS;
    hi2s3.Init.DataFormat = I2S_DATAFORMAT_16B;
    hi2s3.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
    hi2s3.Init.AudioFreq = I2S_AUDIOFREQ_44K;
    hi2s3.Init.CPOL = I2S_CPOL_LOW;
    hi2s3.Init.ClockSource = I2S_CLOCK_PLL;
    hi2s3.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;
    if (HAL_I2S_Init(&hi2s3) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN I2S3_Init 2 */

    /* USER CODE END I2S3_Init 2 */
}

/**
 * @brief RNG Initialization Function
 * @param None
 * @retval None
 */
static void MX_RNG_Init(void)
{

    /* USER CODE BEGIN RNG_Init 0 */

    /* USER CODE END RNG_Init 0 */

    /* USER CODE BEGIN RNG_Init 1 */

    /* USER CODE END RNG_Init 1 */
    hrng.Instance = RNG;
    if (HAL_RNG_Init(&hrng) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN RNG_Init 2 */

    /* USER CODE END RNG_Init 2 */
}

/**
 * @brief TIM2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM2_Init(void)
{

    /* USER CODE BEGIN TIM2_Init 0 */

    /* USER CODE END TIM2_Init 0 */

    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    /* USER CODE BEGIN TIM2_Init 1 */

    /* USER CODE END TIM2_Init 1 */
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 168 - 1;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = (SCROLL_DELAY * 1000) - 1;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
    {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN TIM2_Init 2 */

    /* USER CODE END TIM2_Init 2 */
}

/**
 * Enable DMA controller clock
 */
static void MX_DMA_Init(void)
{

    /* DMA controller clock enable */
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* DMA interrupt init */
    /* DMA1_Stream5_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    /* USER CODE BEGIN MX_GPIO_Init_1 */
    /* USER CODE END MX_GPIO_Init_1 */

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(OTG_FS_PowerSwitchOn_GPIO_Port, OTG_FS_PowerSwitchOn_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOE, KEYBOARD_R4_Pin | KEYBOARD_R3_Pin | KEYBOARD_R2_Pin | KEYBOARD_R1_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOD, LD4_GREEN_Pin | LD3_ORANGE_Pin | LD5_RED_Pin | LD6_BLUE_Pin | AUDIO_RST_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pin : OTG_FS_PowerSwitchOn_Pin */
    GPIO_InitStruct.Pin = OTG_FS_PowerSwitchOn_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(OTG_FS_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pin : USER_BUTTON_Pin */
    GPIO_InitStruct.Pin = USER_BUTTON_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(USER_BUTTON_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pins : KEYBOARD_C1_Pin KEYBOARD_C2_Pin KEYBOARD_C3_Pin KEYBOARD_C4_Pin */
    GPIO_InitStruct.Pin = KEYBOARD_C1_Pin | KEYBOARD_C2_Pin | KEYBOARD_C3_Pin | KEYBOARD_C4_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    /*Configure GPIO pins : KEYBOARD_R4_Pin KEYBOARD_R3_Pin KEYBOARD_R2_Pin KEYBOARD_R1_Pin */
    GPIO_InitStruct.Pin = KEYBOARD_R4_Pin | KEYBOARD_R3_Pin | KEYBOARD_R2_Pin | KEYBOARD_R1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    /*Configure GPIO pins : LD4_GREEN_Pin LD3_ORANGE_Pin LD5_RED_Pin LD6_BLUE_Pin
                             AUDIO_RST_Pin */
    GPIO_InitStruct.Pin = LD4_GREEN_Pin | LD3_ORANGE_Pin | LD5_RED_Pin | LD6_BLUE_Pin | AUDIO_RST_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* EXTI interrupt init*/
    HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);

    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

    /* USER CODE BEGIN MX_GPIO_Init_2 */
    /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_RNG_ReadyDataCallback(RNG_HandleTypeDef *hrng, uint32_t random32bit)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(hrng);
    UNUSED(random32bit);
    /* NOTE : This function should not be modified. When the callback is needed,
              function HAL_RNG_ReadyDataCallback must be implemented in the user file.
     */
    random_number_ready = true;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    current_time = HAL_GetTick();
    if (current_time - previous_time <= 100)
    {
        return;
    }

    GPIO_InitStructPrivate.Pin = KEYBOARD_C1_Pin | KEYBOARD_C2_Pin | KEYBOARD_C3_Pin | KEYBOARD_C4_Pin;
    GPIO_InitStructPrivate.Mode = GPIO_MODE_INPUT;
    GPIO_InitStructPrivate.Pull = GPIO_PULLUP;
    GPIO_InitStructPrivate.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStructPrivate);
    int triggered_col = -1;
    for (uint8_t col = 0; col < NUM_COLS; col++)
    {
        if (GPIO_Pin == COL_PINS[col])
        {
            triggered_col = col;
            break;
        }
    }
    if (triggered_col != -1)
    {
        for (uint8_t row = 0; row < NUM_ROWS; row++)
        {
            HAL_GPIO_WritePin(ROW_PORTS[row], ROW_PINS[row], GPIO_PIN_SET);
        }
        for (uint8_t row = 0; row < NUM_ROWS; row++)
        {
            HAL_GPIO_WritePin(ROW_PORTS[row], ROW_PINS[row], GPIO_PIN_RESET);
            if (!HAL_GPIO_ReadPin(GPIOE, COL_PINS[triggered_col]))
            {
                key_number = KEY_VALUES[row][triggered_col];
                HAL_GPIO_WritePin(ROW_PORTS[row], ROW_PINS[row], GPIO_PIN_SET);
                break;
            }
            HAL_GPIO_WritePin(ROW_PORTS[row], ROW_PINS[row], GPIO_PIN_SET);
        }
    }
    for (uint8_t row = 0; row < NUM_ROWS; row++)
    {
        HAL_GPIO_WritePin(ROW_PORTS[row], ROW_PINS[row], GPIO_PIN_RESET);
    }
    GPIO_InitStructPrivate.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStructPrivate.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStructPrivate);

    previous_time = current_time;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2 && scrolling_active)
    {
        char display[17] = {0};

        for (uint8_t i = 0; i < 16; i++)
        {
            uint8_t text_pos = (current_position + i) % text_length;
            display[i] = scroll_text[text_pos];
        }
        display[16] = '\0';

        lcd16x2_i2c_set_cursor(0, 0);
        lcd16x2_i2c_printf(display);

        current_position = (current_position + 1) % text_length;
    }
}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1)
    {
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
