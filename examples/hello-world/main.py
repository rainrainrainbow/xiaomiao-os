# XiaoMiao OS Hello World App
import xiaomiao
import time

def main():
    lcd = xiaomiao.LCD()
    lcd.clear(0xF6D34A)  # Yellow background
    
    lcd.text(20, 50, "Hello, World!", 0x1B1713)  # Black text
    lcd.text(30, 70, "XiaoMiao OS", 0x5C4220)     # Brown text
    
    # Wait for button press
    buttons = xiaomiao.Buttons()
    while True:
        if buttons.pressed('A'):
            break
        time.sleep(0.1)
    
    lcd.clear(0x000000)

if __name__ == "__main__":
    main()