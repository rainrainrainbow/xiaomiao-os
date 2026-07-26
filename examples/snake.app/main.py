"""
贪吃蛇游戏 - XiaoMiao OS
经典贪吃蛇游戏，使用方向键控制
"""

import time
import display
import buttons

# 屏幕尺寸
SCREEN_W = 160
SCREEN_H = 128

# 游戏区域（留出状态栏）
GAME_X = 0
GAME_Y = 12
GAME_W = 160
GAME_H = 116

# 网格大小
CELL_SIZE = 4
GRID_W = GAME_W // CELL_SIZE  # 40
GRID_H = GAME_H // CELL_SIZE  # 29

# 颜色
COLOR_BG = 0x0000
COLOR_SNAKE = 0x07E0      # 绿色
COLOR_HEAD = 0xFFFF        # 白色
COLOR_FOOD = 0xF800        # 红色
COLOR_GRID = 0x1082        # 深灰
COLOR_TEXT = 0xFFFF

# 方向
DIR_UP = (0, -1)
DIR_DOWN = (0, 1)
DIR_LEFT = (-1, 0)
DIR_RIGHT = (1, 0)


class SnakeGame:
    def __init__(self):
        self.reset()
    
    def reset(self):
        """重置游戏"""
        cx = GRID_W // 2
        cy = GRID_H // 2
        self.snake = [(cx, cy), (cx-1, cy), (cx-2, cy)]
        self.direction = DIR_RIGHT
        self.next_direction = DIR_RIGHT
        self.food = self._spawn_food()
        self.score = 0
        self.game_over = False
        self.speed = 0.15  # 秒/帧
    
    def _spawn_food(self):
        """生成食物"""
        import random
        while True:
            pos = (random.randint(0, GRID_W-1), random.randint(0, GRID_H-1))
            if pos not in self.snake:
                return pos
    
    def handle_input(self, btn):
        """处理按键输入"""
        if self.game_over:
            if btn == buttons.A:
                self.reset()
            return
        
        if btn == buttons.UP and self.direction != DIR_DOWN:
            self.next_direction = DIR_UP
        elif btn == buttons.DOWN and self.direction != DIR_UP:
            self.next_direction = DIR_DOWN
        elif btn == buttons.LEFT and self.direction != DIR_RIGHT:
            self.next_direction = DIR_LEFT
        elif btn == buttons.RIGHT and self.direction != DIR_LEFT:
            self.next_direction = DIR_RIGHT
    
    def update(self):
        """更新游戏状态"""
        if self.game_over:
            return
        
        self.direction = self.next_direction
        
        # 计算新头部位置
        head = self.snake[0]
        new_head = (head[0] + self.direction[0], head[1] + self.direction[1])
        
        # 碰撞检测 - 墙壁
        if (new_head[0] < 0 or new_head[0] >= GRID_W or
            new_head[1] < 0 or new_head[1] >= GRID_H):
            self.game_over = True
            return
        
        # 碰撞检测 - 自身
        if new_head in self.snake:
            self.game_over = True
            return
        
        # 移动蛇
        self.snake.insert(0, new_head)
        
        # 吃食物
        if new_head == self.food:
            self.score += 10
            self.food = self._spawn_food()
            # 加速
            if self.speed > 0.05:
                self.speed -= 0.005
        else:
            self.snake.pop()
    
    def draw(self):
        """绘制游戏画面"""
        display.clear(COLOR_BG)
        
        # 绘制食物
        fx = GAME_X + self.food[0] * CELL_SIZE
        fy = GAME_Y + self.food[1] * CELL_SIZE
        display.fill_rect(fx, fy, CELL_SIZE, CELL_SIZE, COLOR_FOOD)
        
        # 绘制蛇身
        for i, (sx, sy) in enumerate(self.snake):
            px = GAME_X + sx * CELL_SIZE
            py = GAME_Y + sy * CELL_SIZE
            color = COLOR_HEAD if i == 0 else COLOR_SNAKE
            display.fill_rect(px, py, CELL_SIZE, CELL_SIZE, color)
        
        # 绘制分数
        display.text(f"Score:{self.score}", 2, 2, COLOR_TEXT, 1)
        
        # 游戏结束画面
        if self.game_over:
            display.text("GAME OVER", 40, 50, COLOR_FOOD, 2)
            display.text(f"Score: {self.score}", 50, 70, COLOR_TEXT, 1)
            display.text("A:Restart B:Exit", 30, 90, COLOR_GRID, 1)
        
        display.refresh()


def main():
    """贪吃蛇主函数"""
    game = SnakeGame()
    
    while True:
        # 处理按键
        btn = buttons.poll()
        if btn == buttons.B:
            break
        if btn is not None:
            game.handle_input(btn)
        
        # 更新和绘制
        game.update()
        game.draw()
        
        time.sleep(game.speed)


if __name__ == "__main__":
    main()