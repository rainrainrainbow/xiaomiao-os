# XiaoMiao OS Counter App
import xiaomiao
import time

def main():
    lcd = xiaomiao.LCD()
    buttons = xiaomiao.Buttons()
    
    count = 0
    lcd.clear(0xF6D34A)
    lcd.text(40, 30, "Counter", 0x1B1713)
    lcd.text(60, 60, str(count), 0x5C4220)
    lcd.text(20, 100, "A:+1  B:-1", 0x5C4220)
    
    while True:
        if buttons.pressed('A'):
            count += 1
            lcd.rect(50, 55, 60, 20, 0xF6D34A, 0xF6D34A)  # Clear old
            lcd.text(60, 60, str(count), 0x5C4220)
        
        if buttons.pressed('B'):
            count -= 1
            lcd.rect(50, 55, 60, 20, 0xF6D34A, 0xF6D34A)  # Clear old
            lcd.text(60, 60, str(count), 0x5C4220)
        
        if buttons.pressed('UP'):
            break  # Exit app
        
        time.sleep(0.1)
    
    lcd.clear(0x000000)

if __name__ == "__main__":
    main()