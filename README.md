#    C4炸彈安裝拆除模擬器
模擬一個C4炸彈安裝與拆除的專案
##    1.情景故事
此為某款FPS遊戲的C4炸彈，匪徒方輸入**7355608**放置炸彈  
放置炸彈時，**炸彈的紅色燈號閃爍、蜂鳴器運作**  
越靠近炸彈爆炸時會燈號和聲音會閃爍的更快  
警方要阻止炸彈爆炸，如果在爆炸前將炸彈拆除  
炸彈上就會**顯示綠燈，並且紅燈跟蜂鳴器停止運作**  
並且警方如果手上有鉗子，拆除時間會更短   
如果**沒有鉗子，拆除時間是10秒，反之5秒**  
![C4示意圖](Picture/C4.png)  
##    2.腳位對照與思路  
1.使用LPUART1(Baud Rate：115200 , 8N1)在pyserial終端機中輸入文字  
當在**終端機輸入文字”7355608”代表炸彈放置**  
**輸入”CUT”代表你擁有鉗子，拆除速度可以更快。**  
2.使用TIM3 (PWM Generation CH2~4)(PC9 PC8 PA4)(PSC：169 ,ARR：999 ,1ms )  
作為放置炸彈後蜂鳴器鳴叫、紅燈呼吸閃爍、拆炸彈時綠燈進度使用  
3.GPIO_Output (PC6)作為確認是否有”CUT”時使用，如果有就亮燈  
4.GPIO_EXTI(PA11)(Pull-up)中斷程式，作為要拆除炸彈時的按鈕  

--MX設定--  
5.(NVIC EXTI line[15：10] ☑ LPUART1 globle interrupt ☑)  
6.(SYS Debug Serial Wire)  
7.(TIM3 Clock Sourse internal Clock)  
8.(Connectivity Mode Asynchronous)  

--Pyserial設定—  
9.(CMD python -m serial.tools.miniterm --echo COM(你的port號) 115200)  
![MX示意圖](Picture/MX_Pin_Set.png)  
##    3.驗收條件與結果  
1.打開CMD -> Pyserial 執行程式時出現”Hello World” ☑    
2.在Pyserial 輸入7355608 | 紅燈、蜂鳴器呼吸閃爍並倒數 ☑    
3.在Pyserial 輸入CUT | 黃燈亮起 ☑    
4.按下按鈕 | 進入拆除動作 綠燈開始作為進度條亮起 ☑    
5.拆除過程 | 如果有CUT 拆除時間為5秒，如果沒有為10秒 ☑   
6.拆除完畢 | 綠燈常亮，紅燈及蜂鳴器停止 ☑    
7.拆除完畢 | CMD出現 “Defuse successful” ☑   
8.炸彈爆炸 | 綠燈關閉，紅燈及蜂鳴器常開 ☑    
9.炸彈爆炸 | CMD出現 “Booooom”☑   
10.炸彈爆炸後按按鈕 | 不會有反應，炸彈爆炸後無法拆除 ☑      
![MX示意圖](Picture/Example.jpg)  
##    4.C4技術應用勾選表  
☑GPIO輸出 黃燈PC6 MODE=01 判斷是否有鉗子  
☑GPIO輸入 PA11 MODE=00 EXTI來源腳位  
☐複用功能AF   

☑時脈功能 PPL 170MHz  

☑外部中斷 EXTI PC13 一按就準備拆C4  
☑NVIC功能 EXTI line[15:10]  
☐優先權與巢狀 無  
☑軟體去彈跳 按下按鈕拆炸彈時不會誤動作  
  
☑UART 傳送	 傳送Hello world、Bomb SET     
Booom、 Defuse successful來表達目前C4的狀態 	    
☑UART 接收+環形緩衝 接收7355608 與 CUT    
LPUART1 115200 8N1     
☑Pyserial 在CMD裡面輸入文字傳送給STM32    
python -m serial.tools.miniterm - - echo   
COM10 115200    
  
☐Timer 定時中斷  
☑PWM timer3 PSC169 ARR999 1000Hz  
☐輸入捕捉  
##    5.腳位表  
本次應用到的腳位為  
TIM3_CH2 PA4 綠燈 正在拆炸彈、已經拆除完畢炸彈使用  
TIM3_CH3 PC8 紅燈 安裝炸彈後閃爍使用  
TIM3_CH4 PC9 蜂鳴器 安裝炸彈後鳴叫使用  
GPIO_EXTI11 PA11 按鈕 拆除炸彈時使用  
GPIO_Output PC6 黃燈 判斷是否擁有CUT  
LPUART1_TX_STLINKV3E_VCP_RX PA2 接收給STM32 主機傳輸訊息用    
LPUART1_TX_STLINKV3E_VCP_TX PA2 傳輸給主機 主機接收訊息用    
##    6.建置與執行  
**開發環境**：STM32CubeIDE  
**燒錄步驟**：開啟專案 → 點擊 Build (鎚子圖示) → 點擊 Run / Debug 燒錄至 NUCLEO-G474RE。  
**終端機設定與指令**：  
* 開啟終端機：`python -m serial.tools.miniterm --echo COM(你的com號) 115200`  
  * 支援指令：  
    * `7355608`：啟動拆彈倒數與呼吸燈、蜂鳴器。  
    * `CUT`：執行剪線加速指令。  
##    7.程式結構   
7-1主要是程式的宣告、函式庫的使用   
![step7-1](Picture/code1.png)   
7-2這邊的圖為main中 user code2內要放置的程式
啟動timer3 pwm輸出  Lpuart1接收中斷 與發送hello world訊息    
![step7-2](Picture/code2.png) 
7-3 user code3內的程式  
這邊是處理透過硬體中斷把收到的字元一個個塞進（Ring Buffer）中    
這段 while 迴圈會去檢查緩衝區是否有新資料（rx_tail != rx_head），並將其逐一取出  
組合成 Line Buffer：取出的字元會被暫存在 line_buffer 內，直到遇到換行符號（\n或\r）。
![step7-3](Picture/code3.png)  
7-4 user code3內的程式   
當輸入7355608 使C4_SET=1的時候 就開始進行倒計時  
讓紅燈、蜂鳴器開始呼吸運作  
當炸彈越接近爆炸時 呼吸的頻率會越快  
![step7-4](Picture/code4.png) 
7-5 user code3內的程式    
當按下按鈕時就會開始判斷是否處於拆除炸彈狀態，如果擁有鉗子 拆除時間減半(5秒)  
如果拆除成功 就會結束程式 並且讓綠燈常亮("發送Defuse Sueecssful")  
如果沒有在時間內拆除完畢，就拆除失敗爆炸，紅燈及蜂鳴器常亮("發送Booooooom")  
但如果拆除到一半放開，就會作為未拆除成功，炸彈仍繼續倒數。    
![step7-5](Picture/code5.png) 
7-6 UART 接收中斷 (HAL_UART_RxCpltCallback)：自動接收序列埠傳入的每個字元，並存入環形緩衝區   
外部按鈕中斷 (HAL_GPIO_EXTI_Callback)：偵測 PA11 拆彈按鈕的按下動作，觸發拆彈旗標   
指令解析 (parse_command)：判斷接收到的指令，輸入 "7355608" 會啟動 C4 倒數，輸入 "CUT" 則會取得拆彈鉗並點亮黃燈  
![step7-6](Picture/code6.png) 
##    8.實作照片與講解  
###    8-1.MX設定  
8-1-1.打開STM32CubeMX 設定以下腳位：  
PA11：GPIO_EXTI11 | PC9：TIM3_CH4  
PC8：TIM3_CH3 | PC6：GPIO_Output  
PA4：TIM3_CH2  
![step8-1-1](Picture/Execute1.png)  
8-1-2.點選MX左方“System Core”下方的“SYS”  
打開後點擊“Debug”的框  
選擇“Serial Wire  
![step8-1-2](Picture/Execute2.png)   
8-1-3.點選MX左方“Timers”下方的“TIM3”   
打開後點擊“Channel 2,3,4”的框  
選擇“PWM Generation CH2,3,4”  
下方的Preacaler 設定為169  
Counter Period 設定為999  
![step8-1-3](Picture/Execute3.png)  
8-1-4.點選MX左方“Connectivity”下方的“LPUART1”  
打開後點擊“Mode”的框 選擇“Asynchronous”  
下方“Baud Rate”改成“115200 Bit/s”  
![step8-1-4](Picture/Execute4.png)  
8-1-5.點選MX左方“System Core”下方的“NVIC”  
將“EXTI line[15：10] interrupts”打勾  
並且將“Preemption Priority”調整為1  
![step8-1-5](Picture/Execute5.png)  
###    8-2實體配線  
8-2-1.下圖為本次使用接腳的接腳圖，紅色框是 TIM3 的(PA4,PC8,PC9)  
藍色框的是GPIO_EXTI(PA11)、棕色框的是GPIO_Output(PC6)  
![step8-2-1](Picture/Execute6.png)  
8-2-2.此頁為接腳實體配線，黑框為GND，為接地使用  
紅框為(TIM3_CH3,4)，提供給麵包版上的紅燈與蜂鳴器使用  
綠框為(TIM3_CH2)，提供給麵包版上的綠燈使用  
![step8-2-2](Picture/Execute7.png)  
8-2-3.棕框為GPIO_EXTI11，為麵包版上按鈕按下後讀取訊號使用  
黃框為GPIO_Output，提供給麵包版上的黃燈使用  
![step8-2-3](Picture/Execute8.png)  
###    8-3實機操作  
8-3-1.將程式燒錄進STM32前，打開電腦的“裝置管理員”  
查看下方的連接埠，確認自己的STM32在COM幾(此裝置為COM10)  
![step8-3-1](Picture/Execute9.png)  
8-3-2.打開“CMD”後輸入以下字元用來搭建通訊連接  
“python –m serial.tools.miniterm --echo COM10 115200”  
![step8-3-2](Picture/Execute10.png)  
8-3-2-補充.程式執行後，一開始會向 CMD 傳送“Hello World”     
表示目前已經在程式內    
![step8-3-2](Picture/Execute10-1.png)    
8-3-3.在CMD內輸入7355608後 可以看見紅燈及蜂鳴器開始閃爍   
到計時，如果到時間結束還是沒有拆除成功，     
紅燈及蜂鳴器會呈現常亮，不會再進行呼吸閃爍     
![step8-3-3](Picture/Execute11.png)   
8-3-3-補充.如果一段時間內沒有拆除炸彈，則紅燈及蜂鳴器停止閃爍直接常亮，並且往 CMD傳送“Boooooom”      
![step8-3-3](Picture/Execute11-1.png)   
8-3-4.在CMD內輸入CUT後 可以看見黃燈亮起，判斷是否有鉗子   
如果有可以讓拆除時間減少5秒   
![step8-3-4](Picture/Execute12.png)    
8-3-5.在按下按鈕後，會進行拆除炸彈動作，綠燈會逐漸亮起   
(黃燈亮起時拆除時間為5秒，反之10秒)    
![step8-3-5](Picture/Execute13.png)    
8-3-6.拆除完畢後，紅燈及蜂鳴器停止運作，綠燈常亮     
並且向CMD傳送“Defuse Successful”      
![step8-3-6](Picture/Execute14.png)     
##    9.AI使用說明   
9-1.以下為本次詢問AI時使用的文本   
我利用以下文本，使AI生成我需要的C4炸彈放置拆除模擬器
![step9-1](Picture/AI1.png)     
9-2.以下為AI答覆
首先AI先與我確認 STM32CubeMX 硬體腳位與週邊設定  
![step9-2](Picture/AI2.png)  
9-3.接下來都是AI幫我生成的CODE  
根據我的需求擷取我需要的Code移置到我的專案內
![step9-3](Picture/AI3.png)  
![step9-4](Picture/AI4.png) 
![step9-5](Picture/AI5.png) 
![step9-6](Picture/AI6.png) 
9-7.最後是功能說明
![step9-7](Picture/AI7.png)     
##    10.已知限制  
因為本次實作與實際上依然相差甚大，例如C4炸彈 主要成分為RDX旋風炸藥 、 而我們需要透過電流讓雷管通電，引爆內部的初級炸藥    
而雷管爆炸釋放的高溫與高壓衝擊波，這個能量會瞬間激化C4核心的RDX，引發連鎖反應產生爆炸。
因此我們要額外增加一個電雷管、起爆器，當倒數完畢後啟動起爆器，進而引爆C4。    
![step10-1](Picture/Real.jpg)    