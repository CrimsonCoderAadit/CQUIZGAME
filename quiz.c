#include <SDL.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

// Screen dimensions
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 700

// Quiz constants
#define MAX_QUESTIONS 100
#define MAX_QUESTION_LENGTH 256
#define MAX_OPTIONS 4
#define MAX_OPTION_LENGTH 128
#define MAX_NAME_LENGTH 50
#define MAX_PLAYERS 100
#define QUESTIONS_PER_LEVEL 10
#define QUESTION_TIME 30 // 30 seconds per question

// Difficulty levels
#define DIFFICULTY_EASY 0
#define DIFFICULTY_MEDIUM 1
#define DIFFICULTY_HARD 2

// Question structure
typedef struct {
    char question[MAX_QUESTION_LENGTH];
    char options[MAX_OPTIONS][MAX_OPTION_LENGTH];
    int correct_option;
    int difficulty;
} Question;

// Player structure
typedef struct {
    char name[MAX_NAME_LENGTH];
    int scores[3];  // Scores for each difficulty level
} Player;

// Game state structure
typedef struct {
    Question questions[MAX_QUESTIONS];
    int total_questions;
    char current_player[MAX_NAME_LENGTH];
    int current_score[3];  // Scores for each difficulty level
    int time_remaining;
    Uint32 question_start_time;
    Player players[MAX_PLAYERS];
    int total_players;
} GameState;

// Function prototypes
bool init_sdl(SDL_Window** window, SDL_Renderer** renderer, TTF_Font** font);
void close_sdl(SDL_Window* window, SDL_Renderer* renderer, TTF_Font* font);
void render_text(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y, SDL_Color color);
void render_button(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y, int w, int h, SDL_Color bg_color, SDL_Color text_color);
bool is_button_clicked(int mouse_x, int mouse_y, int btn_x, int btn_y, int btn_w, int btn_h);
void get_text_input(SDL_Renderer* renderer, TTF_Font* font, char* buffer, int max_length, const char* prompt);
void render_timer(SDL_Renderer* renderer, TTF_Font* font, int time_remaining, int x, int y);

// Master mode functions
void master_login(SDL_Renderer* renderer, TTF_Font* font, GameState* game);
void add_questions(SDL_Renderer* renderer, TTF_Font* font, GameState* game);
void view_questions(SDL_Renderer* renderer, TTF_Font* font, GameState* game);
void edit_question(SDL_Renderer* renderer, TTF_Font* font, GameState* game, int index);
void delete_question(SDL_Renderer* renderer, TTF_Font* font, GameState* game, int index);
void save_questions(GameState* game);
void load_questions(GameState* game);
void save_players(GameState* game);
void load_players(GameState* game);

// Student mode functions
void student_login(SDL_Renderer* renderer, TTF_Font* font, GameState* game);
void start_quiz(SDL_Renderer* renderer, TTF_Font* font, GameState* game, int difficulty);
void show_results(SDL_Renderer* renderer, TTF_Font* font, GameState* game, int difficulty);
void show_player_history(SDL_Renderer* renderer, TTF_Font* font, GameState* game);
void add_player_score(GameState* game, const char* name, int difficulty, int score);

// Utility functions
void add_default_questions(GameState* game);
void shuffle_questions(Question* questions, int count);

int main(int argc, char* argv[]) {
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    TTF_Font* font = NULL;
    GameState game = {0};

    // Seed random number generator
    srand(time(NULL));

    // Initialize SDL
    if (!init_sdl(&window, &renderer, &font)) {
        return 1;
    }

    // Load or create default questions
    load_questions(&game);
    if (game.total_questions == 0) {
        add_default_questions(&game);
        save_questions(&game);
    }

    // Load player history
    load_players(&game);

    // Colors
    SDL_Color WHITE = {255, 255, 255, 255};
    SDL_Color BLUE = {0, 0, 128, 255};
    SDL_Color LIGHT_BLUE = {100, 149, 237, 255};
    SDL_Color GREEN = {0, 255, 0, 255};
    SDL_Color RED = {255, 0, 0, 255};

    // Main menu
    bool quit = false;
    SDL_Event event;

    while (!quit) {
        SDL_SetRenderDrawColor(renderer, BLUE.r, BLUE.g, BLUE.b, BLUE.a);
        SDL_RenderClear(renderer);

        // Title
        render_text(renderer, font, "QUIZ GAME", SCREEN_WIDTH/2 - 100, 100, WHITE);

        // Master Login Button
        render_button(renderer, font, "Master Login", SCREEN_WIDTH/2 - 100, 250, 200, 50, LIGHT_BLUE, WHITE);

        // Student Login Button
        render_button(renderer, font, "Student Login", SCREEN_WIDTH/2 - 100, 350, 200, 50, LIGHT_BLUE, WHITE);

        // Exit Button
        render_button(renderer, font, "Exit", SCREEN_WIDTH/2 - 100, 450, 200, 50, LIGHT_BLUE, WHITE);

        SDL_RenderPresent(renderer);

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
                break;
            }

            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mouse_x, mouse_y;
                SDL_GetMouseState(&mouse_x, &mouse_y);

                // Master Login Button
                if (is_button_clicked(mouse_x, mouse_y, SCREEN_WIDTH/2 - 100, 250, 200, 50)) {
                    master_login(renderer, font, &game);
                }

                // Student Login Button
                if (is_button_clicked(mouse_x, mouse_y, SCREEN_WIDTH/2 - 100, 350, 200, 50)) {
                    student_login(renderer, font, &game);
                }

                // Exit Button
                if (is_button_clicked(mouse_x, mouse_y, SCREEN_WIDTH/2 - 100, 450, 200, 50)) {
                    quit = true;
                }
            }
        }
    }

    // Cleanup
    close_sdl(window, renderer, font);
    return 0;
}

bool init_sdl(SDL_Window** window, SDL_Renderer** renderer, TTF_Font** font) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return false;
    }
    
    *window = SDL_CreateWindow("Quiz Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                             SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (*window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }
    
    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    if (*renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }
    
    if (TTF_Init() == -1) {
        printf("TTF could not initialize! TTF_Error: %s\n", TTF_GetError());
        return false;
    }
    
    // Try to load a common font - adjust path as needed for your system
    *font = TTF_OpenFont("arial.ttf", 24);
    if (*font == NULL) {
        *font = TTF_OpenFont("dejavu-fonts-ttf-2.37/ttf/DejaVuSans.ttf", 24);
        if (*font == NULL) {
            printf("Failed to load font! TTF_Error: %s\n", TTF_GetError());
            return false;
        }
    }
    
    return true;
}

void close_sdl(SDL_Window* window, SDL_Renderer* renderer, TTF_Font* font) {
    if (font) TTF_CloseFont(font);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}

void render_text(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderText_Solid(font, text, color);
    if (surface == NULL) {
        printf("Unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError());
        return;
    }
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture == NULL) {
        printf("Unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        return;
    }
    
    SDL_Rect dest = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &dest);
    
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void render_button(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y, int w, int h, SDL_Color bg_color, SDL_Color text_color) {
    // Draw button background
    SDL_Rect button_rect = {x, y, w, h};
    SDL_SetRenderDrawColor(renderer, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
    SDL_RenderFillRect(renderer, &button_rect);
    
    // Draw button border
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &button_rect);
    
    // Render button text (centered)
    int text_width, text_height;
    TTF_SizeText(font, text, &text_width, &text_height);
    render_text(renderer, font, text, x + (w - text_width)/2, y + (h - text_height)/2, text_color);
}

bool is_button_clicked(int mouse_x, int mouse_y, int btn_x, int btn_y, int btn_w, int btn_h) {
    return (mouse_x >= btn_x && mouse_x <= btn_x + btn_w &&
            mouse_y >= btn_y && mouse_y <= btn_y + btn_h);
}

void get_text_input(SDL_Renderer* renderer, TTF_Font* font, char* buffer, int max_length, const char* prompt) {
    SDL_Color WHITE = {255, 255, 255, 255};
    SDL_Color BLACK = {0, 0, 0, 255};
    
    memset(buffer, 0, max_length);
    char temp_buffer[MAX_NAME_LENGTH] = {0};
    
    SDL_StartTextInput();
    bool done = false;
    
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_RETURN) {
                        strcpy(buffer, temp_buffer);
                        done = true;
                    } else if (event.key.keysym.sym == SDLK_BACKSPACE && strlen(temp_buffer) > 0) {
                        temp_buffer[strlen(temp_buffer) - 1] = '\0';
                    }
                    break;
                
                case SDL_TEXTINPUT:
                    if (strlen(temp_buffer) < max_length - 1) {
                        strcat(temp_buffer, event.text.text);
                    }
                    break;
                
                case SDL_QUIT:
                    done = true;
                    break;
            }
        }
        
        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 128, 255);
        SDL_RenderClear(renderer);
        
        // Render input prompt
        render_text(renderer, font, prompt, SCREEN_WIDTH/2 - 100, 200, WHITE);
        
        // Render current input
        render_text(renderer, font, temp_buffer, SCREEN_WIDTH/2 - 100, 250, WHITE);
        
        // Render instruction
        render_text(renderer, font, "Press Enter when done", SCREEN_WIDTH/2 - 100, 300, WHITE);
        
        SDL_RenderPresent(renderer);
    }
    
    SDL_StopTextInput();
}

void render_timer(SDL_Renderer* renderer, TTF_Font* font, int time_remaining, int x, int y) {
    SDL_Color WHITE = {255, 255, 255, 255};
    SDL_Color RED = {255, 0, 0, 255};
    
    char timer_text[20];
    sprintf(timer_text, "Time: %d", time_remaining);
    
    // Use red color when time is running low
    SDL_Color color = (time_remaining <= 5) ? RED : WHITE;
    
    render_text(renderer, font, timer_text, x, y, color);
}

void master_login(SDL_Renderer* renderer, TTF_Font* font, GameState* game) {
    SDL_Color WHITE = {255, 255, 255, 255};
    SDL_Color BLUE = {0, 0, 128, 255};
    SDL_Color LIGHT_BLUE = {100, 149, 237, 255};
    SDL_Color RED = {255, 0, 0, 255};
    
    bool quit = false;
    SDL_Event event;
    
    // Password protection
    char password[MAX_NAME_LENGTH] = "admin123"; // Default password
    char input[MAX_NAME_LENGTH] = {0};
    
    // Get password input
    get_text_input(renderer, font, input, MAX_NAME_LENGTH, "Enter Master Password:");
    
    // Check password
    if (strcmp(input, password) != 0) {
        SDL_SetRenderDrawColor(renderer, BLUE.r, BLUE.g, BLUE.b, BLUE.a);
        SDL_RenderClear(renderer);
        render_text(renderer, font, "Incorrect Password!", SCREEN_WIDTH/2 - 100, 250, RED);
        SDL_RenderPresent(renderer);
        SDL_Delay(1500);
        return;
    }
    
    while (!quit) {
        SDL_SetRenderDrawColor(renderer, BLUE.r, BLUE.g, BLUE.b, BLUE.a);
        SDL_RenderClear(renderer);
        
        // Title
        render_text(renderer, font, "MASTER MODE", SCREEN_WIDTH/2 - 100, 100, WHITE);
        
        // Add Questions Button
        render_button(renderer, font, "Add Questions", SCREEN_WIDTH/2 - 100, 200, 200, 50, LIGHT_BLUE, WHITE);
        
        // View Questions Button
        render_button(renderer, font, "View Questions", SCREEN_WIDTH/2 - 100, 300, 200, 50, LIGHT_BLUE, WHITE);
        
        // View Player History Button
        render_button(renderer, font, "View Player History", SCREEN_WIDTH/2 - 100, 400, 200, 50, LIGHT_BLUE, WHITE);
        
        // Back Button
        render_button(renderer, font, "Back to Menu", SCREEN_WIDTH/2 - 100, 500, 200, 50, LIGHT_BLUE, WHITE);
        
        SDL_RenderPresent(renderer);
        
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
                break;
            }
            
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mouse_x, mouse_y;
                SDL_GetMouseState(&mouse_x, &mouse_y);
                
                // Add Questions Button
                if (is_button_clicked(mouse_x, mouse_y, SCREEN_WIDTH/2 - 100, 200, 200, 50)) {
                    add_questions(renderer, font, game);
                }
                
                // View Questions Button
                if (is_button_clicked(mouse_x, mouse_y, SCREEN_WIDTH/2 - 100, 300, 200, 50)) {
                    view_questions(renderer, font, game);
                }
                
                // View Player History Button
                if (is_button_clicked(mouse_x, mouse_y, SCREEN_WIDTH/2 - 100, 400, 200, 50)) {
                    show_player_history(renderer, font, game);
                }
                
                // Back Button
                if (is_button_clicked(mouse_x, mouse_y, SCREEN_WIDTH/2 - 100, 500, 200, 50)) {
                    quit = true;
                }
            }
        }
    }
}

void student_login(SDL_Renderer* renderer, TTF_Font* font, GameState* game) {
    SDL_Color WHITE = {255, 255, 255, 255};
    SDL_Color BLUE = {0, 0, 128, 255};
    SDL_Color LIGHT_BLUE = {100, 149, 237, 255};
    
    // Get student name
    get_text_input(renderer, font, game->current_player, MAX_NAME_LENGTH, "Enter your name:");
    
    bool quit = false;
    SDL_Event event;
    
    while (!quit) {
        SDL_SetRenderDrawColor(renderer, BLUE.r, BLUE.g, BLUE.b, BLUE.a);
        SDL_RenderClear(renderer);
        
        char welcome[100];
        sprintf(welcome, "Welcome, %s!", game->current_player);
        render_text(renderer, font, welcome, SCREEN_WIDTH/2 - 100, 100, WHITE);
        
        // Difficulty Selection Buttons
        render_button(renderer, font, "Easy Quiz", SCREEN_WIDTH/2 - 100, 200, 200, 50, LIGHT_BLUE, WHITE);
        render_button(renderer, font, "Medium Quiz", SCREEN_WIDTH/2 - 100, 300, 200, 50, LIGHT_BLUE, WHITE);
        render_button(renderer, font, "Hard Quiz", SCREEN_WIDTH/2 - 100, 400, 200, 50, LIGHT_BLUE, WHITE);
        
        // View History Button
        render_button(renderer, font, "View History", SCREEN_WIDTH/2 - 100, 500, 200, 50, LIGHT_BLUE, WHITE);
        
        // Back Button
        render_button(renderer, font, "Back to Menu", SCREEN_WIDTH/2 - 100, 600, 200, 50, LIGHT_BLUE, WHITE);
        
        SDL_RenderPresent(renderer);
        
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
                break;
            }
            
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mouse_x, mouse_y;
                SDL_GetMouseState(&mouse_x, &mouse_y);
                
                // Easy Quiz
                if (is_button_clicked(mouse_x, mouse_y, SCREEN_WIDTH/2 - 100, 200, 200, 50)) {
                    start_quiz(renderer, font, game, DIFFICULTY_EASY);
                    show_results(renderer, font, game, DIFFICULTY_EASY);
                    show_player_history(renderer, font, game);
                }
                
                // Medium Quiz
                if (is_button_clicked(mouse_x, mouse_y, SCREEN_WIDTH/2 - 100, 300, 200, 50)) {
                    start_quiz(renderer, font, game, DIFFICULTY_MEDIUM);
                    show_results(renderer, font, game, DIFFICULTY_MEDIUM);
                    show_player_history(renderer, font, game);
                }
                
                // Hard Quiz
                if (is_button_clicked(mouse_x, mouse_y, SCREEN_WIDTH/2 - 100, 400, 200, 50)) {
                    start_quiz(renderer, font, game, DIFFICULTY_HARD);
                    show_results(renderer, font, game, DIFFICULTY_HARD);
                    show_player_history(renderer, font, game);
                }
                
                // View History Button
                if (is_button_clicked(mouse_x, mouse_y, SCREEN_WIDTH/2 - 100, 500, 200, 50)) {
                    show_player_history(renderer, font, game);
                }
                
                // Back Button
                if (is_button_clicked(mouse_x, mouse_y, SCREEN_WIDTH/2 - 100, 600, 200, 50)) {
                    quit = true;
                }
            }
        }
    }
}

void start_quiz(SDL_Renderer* renderer, TTF_Font* font, GameState* game, int difficulty) {
    SDL_Color WHITE = {255, 255, 255, 255};
    SDL_Color BLUE = {0, 0, 128, 255};
    SDL_Color LIGHT_BLUE = {100, 149, 237, 255};
    SDL_Color GREEN = {0, 255, 0, 255};
    SDL_Color RED = {255, 0, 0, 255};
    
    // Filter questions by difficulty
    Question difficulty_questions[MAX_QUESTIONS];
    int count = 0;
    for (int i = 0; i < game->total_questions; i++) {
        if (game->questions[i].difficulty == difficulty) {
            difficulty_questions[count++] = game->questions[i];
        }
    }
    
    if (count < QUESTIONS_PER_LEVEL) {
        SDL_SetRenderDrawColor(renderer, BLUE.r, BLUE.g, BLUE.b, BLUE.a);
        SDL_RenderClear(renderer);
        char message[100];
        sprintf(message, "Not enough questions (%d/%d) for this level!", count, QUESTIONS_PER_LEVEL);
        render_text(renderer, font, message, SCREEN_WIDTH/2 - 250, 250, RED);
        SDL_RenderPresent(renderer);
        SDL_Delay(2000);
        return;
    }
    
    // Shuffle questions
    shuffle_questions(difficulty_questions, count);
    
    // Take only the first QUESTIONS_PER_LEVEL questions
    int score = 0;
    
    // Start quiz
    for (int q = 0; q < QUESTIONS_PER_LEVEL && q < count; q++) {
        Question current_question = difficulty_questions[q];
        bool answered = false;
        int selected_option = -1;
        
        // Start timer for this question
        game->question_start_time = SDL_GetTicks();
        game->time_remaining = QUESTION_TIME;
        
        while (!answered && game->time_remaining > 0) {
            // Calculate remaining time
            Uint32 current_time = SDL_GetTicks();
            game->time_remaining = QUESTION_TIME - (current_time - game->question_start_time) / 1000;
            if (game->time_remaining < 0) game->time_remaining = 0;
            
            SDL_SetRenderDrawColor(renderer, BLUE.r, BLUE.g, BLUE.b, BLUE.a);
            SDL_RenderClear(renderer);
            
            // Display question number
            char question_num[50];
            sprintf(question_num, "Question %d/%d", q + 1, QUESTIONS_PER_LEVEL);
            render_text(renderer, font, question_num, 50, 50, WHITE);
            
            // Display timer
            render_timer(renderer, font, game->time_remaining, SCREEN_WIDTH - 150, 50);
            
            // Display question
            render_text(renderer, font, current_question.question, 50, 100, WHITE);
            
            // Display options
            for (int i = 0; i < MAX_OPTIONS; i++) {
                char option_text[150];
                sprintf(option_text, "%d. %s", i + 1, current_question.options[i]);
                
                // Highlight selected option
                SDL_Color bg_color = (selected_option == i) ? GREEN : LIGHT_BLUE;
                render_button(renderer, font, option_text, 100, 200 + i * 80, 600, 50, bg_color, WHITE);
            }
            
            // Submit button
            if (selected_option != -1) {
                render_button(renderer, font, "Submit Answer", SCREEN_WIDTH/2 - 100, 550, 200, 50, GREEN, WHITE);
            }
            
            SDL_RenderPresent(renderer);
            
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    return;
                }
                
                if (event.type == SDL_MOUSEBUTTONDOWN) {
                    int mouse_x, mouse_y;
                    SDL_GetMouseState(&mouse_x, &mouse_y);
                    
                    // Check option buttons
                    for (int i = 0; i < MAX_OPTIONS; i++) {
                        if (is_button_clicked(mouse_x, mouse_y, 100, 200 + i * 80, 600, 50)) {
                            selected_option = i;
                        }
                    }
                    
                    // Submit button
                    if (selected_option != -1 && is_button_clicked(mouse_x, mouse_y, SCREEN_WIDTH/2 - 100, 550, 200, 50)) {
                        answered = true;
                        
                        // Check answer
                        if (selected_option == current_question.correct_option) {
                            score += 5; // Correct answer: +5 points
                        } else {
                            score -= 1; // Incorrect answer: -1 point
                        }
                    }
                }
            }
        }
        
        // Time's up
        if (!answered && game->time_remaining <= 0) {
            SDL_SetRenderDrawColor(renderer, BLUE.r, BLUE.g, BLUE.b, BLUE.a);
            SDL_RenderClear(renderer);
            render_text(renderer, font, "Time's up!", SCREEN_WIDTH/2 - 100, 250, RED);
            
            // Show correct answer
            char correct_answer[100];
            sprintf(correct_answer, "Correct answer: %d", current_question.correct_option + 1);
            render_text(renderer, font, correct_answer, SCREEN_WIDTH/2 - 100, 300, GREEN);
            
            SDL_RenderPresent(renderer);
            SDL_Delay(2000);
        }
    }
    
    // Store score for this difficulty
    game->current_score[difficulty] = score;
    
    // Add to player history
    add_player_score(game, game->current_player, difficulty, score);
}

void show_results(SDL_Renderer* renderer, TTF_Font* font, GameState* game, int difficulty) {
    SDL_Color WHITE = {255, 255, 255, 255};
    SDL_Color BLUE = {0, 0, 128, 255};
    SDL_Color GREEN = {0, 255, 0, 255};
    SDL_Color RED = {255, 0, 0, 255};
    
    const char* difficulty_str;
    switch (difficulty) {
        case DIFFICULTY_EASY: difficulty_str = "Easy"; break;
        case DIFFICULTY_MEDIUM: difficulty_str = "Medium"; break;
        case DIFFICULTY_HARD: difficulty_str = "Hard"; break;
        default: difficulty_str = "Unknown";
    }
    
    SDL_SetRenderDrawColor(renderer, BLUE.r, BLUE.g, BLUE.b, BLUE.a);
    SDL_RenderClear(renderer);
    
    // Title
    char title[100];
    sprintf(title, "%s Quiz Results", difficulty_str);
    render_text(renderer, font, title, SCREEN_WIDTH/2 - 100, 100, WHITE);
    
    // Player Name
    char name_text[100];
    sprintf(name_text, "Player: %s", game->current_player);
    render_text(renderer, font, name_text, SCREEN_WIDTH/2 - 100, 150, WHITE);
    
    // Score
    char score_text[50];
    sprintf(score_text, "Score: %d", game->current_score[difficulty]);
    render_text(renderer, font, score_text, SCREEN_WIDTH/2 - 100, 200, 
               game->current_score[difficulty] >= 0 ? GREEN : RED);
    
    // Percentage
    float percentage = (float)game->current_score[difficulty] / (QUESTIONS_PER_LEVEL * 5) * 100;
    char percentage_text[50];
    sprintf(percentage_text, "Percentage: %.1f%%", percentage);
    render_text(renderer, font, percentage_text, SCREEN_WIDTH/2 - 100, 250, WHITE);
    
    // Back Button
    render_button(renderer, font, "Continue", SCREEN_WIDTH/2 - 100, 350, 200, 50, GREEN, WHITE);
    
    SDL_RenderPresent(renderer);
    
    // Wait for back button
    bool done = false;
    SDL_Event event;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                done = true;
            }
            
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mouse_x, mouse_y;
                SDL_GetMouseState(&mouse_x, &mouse_y);
                
                if (is_button_clicked(mouse_x, mouse_y, SCREEN_WIDTH/2 - 100, 350, 200, 50)) {
                    done = true;
                }
            }
        }
    }
}

void show_player_history(SDL_Renderer* renderer, TTF_Font* font, GameState* game) {
    SDL_Color WHITE = {255, 255, 255, 255};
    SDL_Color BLUE = {0, 0, 128, 255};
    SDL_Color GREEN = {0, 255, 0, 255};
    SDL_Color RED = {255, 0, 0, 255};
    SDL_Color LIGHT_BLUE = {100, 149, 237, 255};
    
    bool quit = false;
    int start_index = 0;
    int players_per_page = 5;
    
    while (!quit) {
        SDL_SetRenderDrawColor(renderer, BLUE.r, BLUE.g, BLUE.b, BLUE.a);
        SDL_RenderClear(renderer);
        
        // Title
        render_text(renderer, font, "Player History", SCREEN_WIDTH/2 - 100, 50, WHITE);
        
        // Column headers
        render_text(renderer, font, "Player", 50, 100, WHITE);
        render_text(renderer, font, "Easy", 300, 100, WHITE);
        render_text(renderer, font, "Medium", 400, 100, WHITE);
        render_text(renderer, font, "Hard", 500, 100, WHITE);
        
        // Display players
        for (int i = 0; i < players_per_page && (start_index + i) < game->total_players; i++) {
            Player p = game->players[start_index + i];
            
            // Player name
            render_text(renderer, font, p.name, 50, 150 + i * 50, WHITE);
            
            // Scores
            char easy_score[10];
            sprintf(easy_score, "%d", p.scores[DIFFICULTY_EASY]);
            render_text(renderer, font, easy_score, 300, 150 + i * 50, 
                       p.scores[DIFFICULTY_EASY] >= 0 ? GREEN : RED);
            
            char medium_score[10];
            sprintf(medium_score, "%d", p.scores[DIFFICULTY_MEDIUM]);
            render_text(renderer, font, medium_score, 400, 150 + i * 50, 
                       p.scores[DIFFICULTY_MEDIUM] >= 0 ? GREEN : RED);
            
            char hard_score[10];
            sprintf(hard_score, "%d", p.scores[DIFFICULTY_HARD]);
            render_text(renderer, font, hard_score, 500, 150 + i * 50, 
                       p.scores[DIFFICULTY_HARD] >= 0 ? GREEN : RED);
        }
        
        // Navigation buttons
        if (start_index > 0) {
            render_button(renderer, font, "Previous", 50, 450, 150, 50, LIGHT_BLUE, WHITE);
        }
        
        if (start_index + players_per_page < game->total_players) {
            render_button(renderer, font, "Next", SCREEN_WIDTH - 200, 450, 150, 50, LIGHT_BLUE, WHITE);
        }
        
        // Back button
        render_button(renderer, font, "Back", SCREEN_WIDTH/2 - 75, 520, 150, 50, GREEN, WHITE);
        
        SDL_RenderPresent(renderer);
        
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }
            
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mouse_x, mouse_y;
                SDL_GetMouseState(&mouse_x, &mouse_y);
                
                // Previous button
                if (start_index > 0 && is_button_clicked(mouse_x, mouse_y, 50, 450, 150, 50)) {
                    start_index -= players_per_page;
                    if (start_index < 0) start_index = 0;
                }
                
                // Next button
                if (start_index + players_per_page < game->total_players && 
                    is_button_clicked(mouse_x, mouse_y, SCREEN_WIDTH - 200, 450, 150, 50)) {
                    start_index += players_per_page;
                }
                
                // Back button
                if (is_button_clicked(mouse_x, mouse_y, SCREEN_WIDTH/2 - 75, 520, 150, 50)) {
                    quit = true;
                }
            }
        }
    }
}

void add_player_score(GameState* game, const char* name, int difficulty, int score) {
    // Check if player already exists
    int player_index = -1;
    for (int i = 0; i < game->total_players; i++) {
        if (strcmp(game->players[i].name, name) == 0) {
            player_index = i;
            break;
        }
    }
    
    if (player_index != -1) {
        // Update existing player's score for this difficulty
        game->players[player_index].scores[difficulty] = score;
    } else if (game->total_players < MAX_PLAYERS) {
        // Add new player
        strcpy(game->players[game->total_players].name, name);
        // Initialize all scores to -1 (not attempted)
        for (int i = 0; i < 3; i++) {
            game->players[game->total_players].scores[i] = -1;
        }
        // Set the current difficulty score
        game->players[game->total_players].scores[difficulty] = score;
        game->total_players++;
    }
    
    // Save the updated player data
    save_players(game);
}

void add_questions(SDL_Renderer* renderer, TTF_Font* font, GameState* game) {
    SDL_Color WHITE = {255, 255, 255, 255};
    SDL_Color BLUE = {0, 0, 128, 255};
    SDL_Color LIGHT_BLUE = {100, 149, 237, 255};
    SDL_Color GREEN = {0, 255, 0, 255};
    SDL_Color RED = {255, 0, 0, 255};
    
    // Check if question limit is reached
    if (game->total_questions >= MAX_QUESTIONS) {
        SDL_SetRenderDrawColor(renderer, BLUE.r, BLUE.g, BLUE.b, BLUE.a);
        SDL_RenderClear(renderer);
        render_text(renderer, font, "Question limit reached!", SCREEN_WIDTH/2 - 150, 250, RED);
        SDL_RenderPresent(renderer);
        SDL_Delay(1500);
        return;
    }
    
    Question new_question = {0};
    
    // Select Difficulty
    bool difficulty_selected = false;
    while (!difficulty_selected) {
        SDL_SetRenderDrawColor(renderer, BLUE.r, BLUE.g, BLUE.b, BLUE.a);
        SDL_RenderClear(renderer);
        
        render_text(renderer, font, "Select Difficulty", SCREEN_WIDTH/2 - 100, 100, WHITE);
        
        render_button(renderer, font, "Easy", SCREEN_WIDTH/2 - 100, 200, 200, 50, LIGHT_BLUE, WHITE);
        render_button(renderer, font, "Medium", SCREEN_WIDTH/2 - 100, 300, 200, 50, LIGHT_BLUE, WHITE);
        render_button(renderer, font, "Hard", SCREEN_WIDTH/2 - 100, 400, 200, 50, LIGHT_BLUE, WHITE);
        
        SDL_RenderPresent(renderer);
        
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                return;
            }
            
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mouse_x, mouse_y;
                SDL_GetMouseState(&mouse_x, &mouse_y);
                
                if (is_button_clicked(mouse_x, mouse_y, SCREEN_WIDTH/2 - 100, 200, 200, 50)) {
                    new_question.difficulty = DIFFICULTY_EASY;
                    difficulty_selected = true;
                }
                if (is_button_clicked(mouse_x, mouse_y, SCREEN_WIDTH/2 - 100, 300, 200, 50)) {
                    new_question.difficulty = DIFFICULTY_MEDIUM;
                    difficulty_selected = true;
                }
                if (is_button_clicked(mouse_x, mouse_y, SCREEN_WIDTH/2 - 100, 400, 200, 50)) {
                    new_question.difficulty = DIFFICULTY_HARD;
                    difficulty_selected = true;
                }
            }
        }
    }
    
    // Enter Question
    SDL_SetRenderDrawColor(renderer, BLUE.r, BLUE.g, BLUE.b, BLUE.a);
    SDL_RenderClear(renderer);
    render_text(renderer, font, "Enter Question", SCREEN_WIDTH/2 - 100, 100, WHITE);
    SDL_RenderPresent(renderer);
    
    char question_input[MAX_QUESTION_LENGTH];
    get_text_input(renderer, font, question_input, MAX_QUESTION_LENGTH, "Enter the question:");
    strcpy(new_question.question, question_input);
    
    // Enter Options
    for (int i = 0; i < MAX_OPTIONS; i++) {
        SDL_SetRenderDrawColor(renderer, BLUE.r, BLUE.g, BLUE.b, BLUE.a);
        SDL_RenderClear(renderer);
        
        char option_prompt[100];
        sprintf(option_prompt, "Enter Option %d", i + 1);
        render_text(renderer, font, option_prompt, SCREEN_WIDTH/2 - 100, 100, WHITE);
        SDL_RenderPresent(renderer);
        
        char option_input[MAX_OPTION_LENGTH];
        get_text_input(renderer, font, option_input, MAX_OPTION_LENGTH, option_prompt);
        strcpy(new_question.options[i], option_input);
    }
    
    // Select Correct Option
    bool correct_selected = false;
    while (!correct_selected) {
        SDL_SetRenderDrawColor(renderer, BLUE.r, BLUE.g, BLUE.b, BLUE.a);
        SDL_RenderClear(renderer);
        
        render_text(renderer, font, "Select Correct Option", SCREEN_WIDTH/2 - 100, 100, WHITE);
        
        for (int i = 0; i < MAX_OPTIONS; i++) {
            char button_text[MAX_OPTION_LENGTH + 10];
            sprintf(button_text, "%d. %s", i + 1, new_question.options[i]);
            render_button(renderer, font, button_text, SCREEN_WIDTH/2 - 100, 200 + i * 80, 200, 50, LIGHT_BLUE, WHITE);
        }
        
        SDL_RenderPresent(renderer);
        
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                return;
            }
            
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mouse_x, mouse_y;
                SDL_GetMouseState(&mouse_x, &mouse_y);
                
                for (int i = 0; i < MAX_OPTIONS; i++) {
                    if (is_button_clicked(mouse_x, mouse_y, SCREEN_WIDTH/2 - 100, 200 + i * 80, 200, 50)) {
                        new_question.correct_option = i;
                        correct_selected = true;
                        break;
                    }
                }
            }
        }
    }
    
    // Add question to game
    game->questions[game->total_questions++] = new_question;
    
    // Save questions
    save_questions(game);
    
    // Confirmation
    SDL_SetRenderDrawColor(renderer, BLUE.r, BLUE.g, BLUE.b, BLUE.a);
    SDL_RenderClear(renderer);
    render_text(renderer, font, "Question Added Successfully!", SCREEN_WIDTH/2 - 150, 250, GREEN);
    SDL_RenderPresent(renderer);
    SDL_Delay(1500);
}

void view_questions(SDL_Renderer* renderer, TTF_Font* font, GameState* game) {
    SDL_Color WHITE = {255, 255, 255, 255};
    SDL_Color BLUE = {0, 0, 128, 255};
    SDL_Color LIGHT_BLUE = {100, 149, 237, 255};
    SDL_Color GREEN = {0, 255, 0, 255};
    SDL_Color RED = {255, 0, 0, 255};
    
    int current_index = 0;
    bool quit = false;
    SDL_Event event;
    
    while (!quit && current_index < game->total_questions) {
        SDL_SetRenderDrawColor(renderer, BLUE.r, BLUE.g, BLUE.b, BLUE.a);
        SDL_RenderClear(renderer);
        
        // Display question number
        char question_num[50];
        sprintf(question_num, "Question %d/%d", current_index + 1, game->total_questions);
        render_text(renderer, font, question_num, SCREEN_WIDTH/2 - 100, 50, WHITE);
        
        // Display difficulty
        const char* difficulty_str;
        switch (game->questions[current_index].difficulty) {
            case DIFFICULTY_EASY: difficulty_str = "Easy"; break;
            case DIFFICULTY_MEDIUM: difficulty_str = "Medium"; break;
            case DIFFICULTY_HARD: difficulty_str = "Hard"; break;
            default: difficulty_str = "Unknown";
        }
        render_text(renderer, font, difficulty_str, SCREEN_WIDTH - 150, 50, WHITE);
        
        // Display question
        render_text(renderer, font, game->questions[current_index].question, 50, 100, WHITE);
        
        // Display options
        for (int i = 0; i < MAX_OPTIONS; i++) {
            char option_text[150];
            sprintf(option_text, "%d. %s", i + 1, game->questions[current_index].options[i]);
            render_text(renderer, font, option_text, 100, 200 + i * 50, WHITE);
        }
        
        // Highlight correct answer
        char correct_text[100];
        sprintf(correct_text, "Correct Answer: %d", game->questions[current_index].correct_option + 1);
        render_text(renderer, font, correct_text, 50, 400, GREEN);
        
        // Navigation buttons
        if (current_index > 0) {
            render_button(renderer, font, "Previous", 50, 500, 150, 50, LIGHT_BLUE, WHITE);
        }
        if (current_index < game->total_questions - 1) {
            render_button(renderer, font, "Next", SCREEN_WIDTH - 200, 500, 150, 50, LIGHT_BLUE, WHITE);
        }
        
        // Edit and Delete buttons
        render_button(renderer, font, "Edit", SCREEN_WIDTH/2 - 75, 500, 150, 50, LIGHT_BLUE, WHITE);
        render_button(renderer, font, "Delete", SCREEN_WIDTH/2 - 75, 570, 150, 50, RED, WHITE);
        
        // Back button
        render_button(renderer, font, "Back", SCREEN_WIDTH/2 - 75, 640, 150, 50, LIGHT_BLUE, WHITE);
        
        SDL_RenderPresent(renderer);
        
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
                break;
            }
            
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mouse_x, mouse_y;
                SDL_GetMouseState(&mouse_x, &mouse_y);
                
                // Previous button
                if (current_index > 0 && is_button_clicked(mouse_x, mouse_y, 50, 500, 150, 50)) {
                    current_index--;
                }
                
                // Next button
                if (current_index < game->total_questions - 1 && is_button_clicked(mouse_x, mouse_y, SCREEN_WIDTH - 200, 500, 150, 50)) {
                    current_index++;
                }
                
                // Edit button
                if (is_button_clicked(mouse_x, mouse_y, SCREEN_WIDTH/2 - 75, 500, 150, 50)) {
                    edit_question(renderer, font, game, current_index);
                }
                
                // Delete button
                if (is_button_clicked(mouse_x, mouse_y, SCREEN_WIDTH/2 - 75, 570, 150, 50)) {
                    delete_question(renderer, font, game, current_index);
                    if (current_index >= game->total_questions) {
                        current_index = game->total_questions - 1;
                    }
                }
                
                // Back button
                if (is_button_clicked(mouse_x, mouse_y, SCREEN_WIDTH/2 - 75, 640, 150, 50)) {
                    quit = true;
                }
            }
        }
    }
}

void edit_question(SDL_Renderer* renderer, TTF_Font* font, GameState* game, int index) {
    SDL_Color WHITE = {255, 255, 255, 255};
    SDL_Color BLUE = {0, 0, 128, 255};
    SDL_Color LIGHT_BLUE = {100, 149, 237, 255};
    SDL_Color GREEN = {0, 255, 0, 255};
    
    Question* question = &game->questions[index];
    
    // Select what to edit
    bool done = false;
    while (!done) {
        SDL_SetRenderDrawColor(renderer, BLUE.r, BLUE.g, BLUE.b, BLUE.a);
        SDL_RenderClear(renderer);
        
        render_text(renderer, font, "Edit Question", SCREEN_WIDTH/2 - 100, 50, WHITE);
        
        render_button(renderer, font, "Edit Question Text", SCREEN_WIDTH/2 - 150, 150, 300, 50, LIGHT_BLUE, WHITE);
        
        for (int i = 0; i < MAX_OPTIONS; i++) {
            char button_text[50];
            sprintf(button_text, "Edit Option %d", i + 1);
            render_button(renderer, font, button_text, SCREEN_WIDTH/2 - 150, 220 + i * 70, 300, 50, LIGHT_BLUE, WHITE);
        }
        
        render_button(renderer, font, "Change Correct Answer", SCREEN_WIDTH/2 - 150, 500, 300, 50, LIGHT_BLUE, WHITE);
        
        render_button(renderer, font, "Done", SCREEN_WIDTH/2 - 150, 580, 300, 50, GREEN, WHITE);
        
        SDL_RenderPresent(renderer);
        
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                done = true;
                break;
            }
            
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mouse_x, mouse_y;
                SDL_GetMouseState(&mouse_x, &mouse_y);
                
                // Edit Question Text
                if (is_button_clicked(mouse_x, mouse_y, SCREEN_WIDTH/2 - 150, 150, 300, 50)) {
                    char new_question[MAX_QUESTION_LENGTH];
                    get_text_input(renderer, font, new_question, MAX_QUESTION_LENGTH, "Enter new question text:");
                    strcpy(question->question, new_question);
                }
                
                // Edit Options
                for (int i = 0; i < MAX_OPTIONS; i++) {
                    if (is_button_clicked(mouse_x, mouse_y, SCREEN_WIDTH/2 - 150, 220 + i * 70, 300, 50)) {
                        char new_option[MAX_OPTION_LENGTH];
                        get_text_input(renderer, font, new_option, MAX_OPTION_LENGTH, "Enter new option text:");
                        strcpy(question->options[i], new_option);
                    }
                }
                
                // Change Correct Answer
                if (is_button_clicked(mouse_x, mouse_y, SCREEN_WIDTH/2 - 150, 500, 300, 50)) {
                    bool correct_selected = false;
                    while (!correct_selected) {
                        SDL_SetRenderDrawColor(renderer, BLUE.r, BLUE.g, BLUE.b, BLUE.a);
                        SDL_RenderClear(renderer);
                        
                        render_text(renderer, font, "Select Correct Option", SCREEN_WIDTH/2 - 100, 100, WHITE);
                        
                        for (int i = 0; i < MAX_OPTIONS; i++) {
                            char button_text[MAX_OPTION_LENGTH + 10];
                            sprintf(button_text, "%d. %s", i + 1, question->options[i]);
                            render_button(renderer, font, button_text, SCREEN_WIDTH/2 - 100, 200 + i * 80, 200, 50, LIGHT_BLUE, WHITE);
                        }
                        
                        SDL_RenderPresent(renderer);
                        
                        SDL_Event event;
                        while (SDL_PollEvent(&event)) {
                            if (event.type == SDL_QUIT) {
                                correct_selected = true;
                                break;
                            }
                            
                            if (event.type == SDL_MOUSEBUTTONDOWN) {
                                int mouse_x, mouse_y;
                                SDL_GetMouseState(&mouse_x, &mouse_y);
                                
                                for (int i = 0; i < MAX_OPTIONS; i++) {
                                    if (is_button_clicked(mouse_x, mouse_y, SCREEN_WIDTH/2 - 100, 200 + i * 80, 200, 50)) {
                                        question->correct_option = i;
                                        correct_selected = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
                
                // Done button
                if (is_button_clicked(mouse_x, mouse_y, SCREEN_WIDTH/2 - 150, 580, 300, 50)) {
                    done = true;
                }
            }
        }
    }
    
    // Save changes
    save_questions(game);
    
    // Confirmation
    SDL_SetRenderDrawColor(renderer, BLUE.r, BLUE.g, BLUE.b, BLUE.a);
    SDL_RenderClear(renderer);
    render_text(renderer, font, "Question Updated Successfully!", SCREEN_WIDTH/2 - 150, 250, GREEN);
    SDL_RenderPresent(renderer);
    SDL_Delay(1500);
}

void delete_question(SDL_Renderer* renderer, TTF_Font* font, GameState* game, int index) {
    SDL_Color WHITE = {255, 255, 255, 255};
    SDL_Color BLUE = {0, 0, 128, 255};
    SDL_Color RED = {255, 0, 0, 255};
    SDL_Color GREEN = {0, 255, 0, 255};
    
    // Confirm deletion
    bool confirmed = false;
    bool quit = false;
    
    while (!quit) {
        SDL_SetRenderDrawColor(renderer, BLUE.r, BLUE.g, BLUE.b, BLUE.a);
        SDL_RenderClear(renderer);
        
        render_text(renderer, font, "Are you sure you want to delete this question?", SCREEN_WIDTH/2 - 250, 200, WHITE);
        
        render_button(renderer, font, "Yes", SCREEN_WIDTH/2 - 150, 300, 100, 50, RED, WHITE);
        render_button(renderer, font, "No", SCREEN_WIDTH/2 + 50, 300, 100, 50, WHITE, BLUE);
        
        SDL_RenderPresent(renderer);
        
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
                break;
            }
            
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mouse_x, mouse_y;
                SDL_GetMouseState(&mouse_x, &mouse_y);
                
                // Yes button
                if (is_button_clicked(mouse_x, mouse_y, SCREEN_WIDTH/2 - 150, 300, 100, 50)) {
                    confirmed = true;
                    quit = true;
                }
                
                // No button
                if (is_button_clicked(mouse_x, mouse_y, SCREEN_WIDTH/2 + 50, 300, 100, 50)) {
                    quit = true;
                }
            }
        }
    }
    
    if (confirmed) {
        // Shift all questions after the deleted one
        for (int i = index; i < game->total_questions - 1; i++) {
            game->questions[i] = game->questions[i + 1];
        }
        game->total_questions--;
        
        // Save changes
        save_questions(game);
        
        // Confirmation
        SDL_SetRenderDrawColor(renderer, BLUE.r, BLUE.g, BLUE.b, BLUE.a);
        SDL_RenderClear(renderer);
        render_text(renderer, font, "Question Deleted Successfully!", SCREEN_WIDTH/2 - 150, 250, GREEN);
        SDL_RenderPresent(renderer);
        SDL_Delay(1500);
    }
}

void save_questions(GameState* game) {
    FILE* file = fopen("quiz_questions.dat", "wb");
    if (file) {
        fwrite(&game->total_questions, sizeof(int), 1, file);
        fwrite(game->questions, sizeof(Question), game->total_questions, file);
        fclose(file);
    }
}

void load_questions(GameState* game) {
    FILE* file = fopen("quiz_questions.dat", "rb");
    if (file) {
        fread(&game->total_questions, sizeof(int), 1, file);
        fread(game->questions, sizeof(Question), game->total_questions, file);
        fclose(file);
    }
}

void save_players(GameState* game) {
    FILE* file = fopen("quiz_players.dat", "wb");
    if (file) {
        fwrite(&game->total_players, sizeof(int), 1, file);
        fwrite(game->players, sizeof(Player), game->total_players, file);
        fclose(file);
    }
}

void load_players(GameState* game) {
    FILE* file = fopen("quiz_players.dat", "rb");
    if (file) {
        fread(&game->total_players, sizeof(int), 1, file);
        fread(game->players, sizeof(Player), game->total_players, file);
        fclose(file);
    }
}

void add_default_questions(GameState* game) {
    // Easy Questions
    strcpy(game->questions[game->total_questions].question, "What is 2 + 2?");
    strcpy(game->questions[game->total_questions].options[0], "3");
    strcpy(game->questions[game->total_questions].options[1], "4");
    strcpy(game->questions[game->total_questions].options[2], "5");
    strcpy(game->questions[game->total_questions].options[3], "6");
    game->questions[game->total_questions].correct_option = 1;
    game->questions[game->total_questions].difficulty = DIFFICULTY_EASY;
    game->total_questions++;

    strcpy(game->questions[game->total_questions].question, "What is the capital of France?");
    strcpy(game->questions[game->total_questions].options[0], "London");
    strcpy(game->questions[game->total_questions].options[1], "Berlin");
    strcpy(game->questions[game->total_questions].options[2], "Paris");
    strcpy(game->questions[game->total_questions].options[3], "Madrid");
    game->questions[game->total_questions].correct_option = 2;
    game->questions[game->total_questions].difficulty = DIFFICULTY_EASY;
    game->total_questions++;

    // Add more easy questions to reach at least 10
    strcpy(game->questions[game->total_questions].question, "Which planet is closest to the sun?");
    strcpy(game->questions[game->total_questions].options[0], "Venus");
    strcpy(game->questions[game->total_questions].options[1], "Mars");
    strcpy(game->questions[game->total_questions].options[2], "Mercury");
    strcpy(game->questions[game->total_questions].options[3], "Earth");
    game->questions[game->total_questions].correct_option = 2;
    game->questions[game->total_questions].difficulty = DIFFICULTY_EASY;
    game->total_questions++;

    strcpy(game->questions[game->total_questions].question, "How many continents are there?");
    strcpy(game->questions[game->total_questions].options[0], "5");
    strcpy(game->questions[game->total_questions].options[1], "6");
    strcpy(game->questions[game->total_questions].options[2], "7");
    strcpy(game->questions[game->total_questions].options[3], "8");
    game->questions[game->total_questions].correct_option = 2;
    game->questions[game->total_questions].difficulty = DIFFICULTY_EASY;
    game->total_questions++;

    strcpy(game->questions[game->total_questions].question, "What is the largest ocean on Earth?");
    strcpy(game->questions[game->total_questions].options[0], "Atlantic");
    strcpy(game->questions[game->total_questions].options[1], "Indian");
    strcpy(game->questions[game->total_questions].options[2], "Arctic");
    strcpy(game->questions[game->total_questions].options[3], "Pacific");
    game->questions[game->total_questions].correct_option = 3;
    game->questions[game->total_questions].difficulty = DIFFICULTY_EASY;
    game->total_questions++;

    // Medium Questions
    strcpy(game->questions[game->total_questions].question, "What is the square root of 64?");
    strcpy(game->questions[game->total_questions].options[0], "4");
    strcpy(game->questions[game->total_questions].options[1], "6");
    strcpy(game->questions[game->total_questions].options[2], "8");
    strcpy(game->questions[game->total_questions].options[3], "10");
    game->questions[game->total_questions].correct_option = 2;
    game->questions[game->total_questions].difficulty = DIFFICULTY_MEDIUM;
    game->total_questions++;

    strcpy(game->questions[game->total_questions].question, "Which planet is known as the Red Planet?");
    strcpy(game->questions[game->total_questions].options[0], "Venus");
    strcpy(game->questions[game->total_questions].options[1], "Mars");
    strcpy(game->questions[game->total_questions].options[2], "Jupiter");
    strcpy(game->questions[game->total_questions].options[3], "Saturn");
    game->questions[game->total_questions].correct_option = 1;
    game->questions[game->total_questions].difficulty = DIFFICULTY_MEDIUM;
    game->total_questions++;

    // Add more medium questions to reach at least 10
    strcpy(game->questions[game->total_questions].question, "What is the chemical symbol for water?");
    strcpy(game->questions[game->total_questions].options[0], "H2O");
    strcpy(game->questions[game->total_questions].options[1], "CO2");
    strcpy(game->questions[game->total_questions].options[2], "NaCl");
    strcpy(game->questions[game->total_questions].options[3], "O2");
    game->questions[game->total_questions].correct_option = 0;
    game->questions[game->total_questions].difficulty = DIFFICULTY_MEDIUM;
    game->total_questions++;

    strcpy(game->questions[game->total_questions].question, "Who wrote 'Romeo and Juliet'?");
    strcpy(game->questions[game->total_questions].options[0], "Charles Dickens");
    strcpy(game->questions[game->total_questions].options[1], "William Shakespeare");
    strcpy(game->questions[game->total_questions].options[2], "Jane Austen");
    strcpy(game->questions[game->total_questions].options[3], "Mark Twain");
    game->questions[game->total_questions].correct_option = 1;
    game->questions[game->total_questions].difficulty = DIFFICULTY_MEDIUM;
    game->total_questions++;

    strcpy(game->questions[game->total_questions].question, "What is the capital of Japan?");
    strcpy(game->questions[game->total_questions].options[0], "Beijing");
    strcpy(game->questions[game->total_questions].options[1], "Seoul");
    strcpy(game->questions[game->total_questions].options[2], "Tokyo");
    strcpy(game->questions[game->total_questions].options[3], "Bangkok");
    game->questions[game->total_questions].correct_option = 2;
    game->questions[game->total_questions].difficulty = DIFFICULTY_MEDIUM;
    game->total_questions++;

    // Hard Questions
    strcpy(game->questions[game->total_questions].question, "What is the chemical symbol for Gold?");
    strcpy(game->questions[game->total_questions].options[0], "Go");
    strcpy(game->questions[game->total_questions].options[1], "Gd");
    strcpy(game->questions[game->total_questions].options[2], "Au");
    strcpy(game->questions[game->total_questions].options[3], "Ag");
    game->questions[game->total_questions].correct_option = 2;
    game->questions[game->total_questions].difficulty = DIFFICULTY_HARD;
    game->total_questions++;

    strcpy(game->questions[game->total_questions].question, "Who painted the Mona Lisa?");
    strcpy(game->questions[game->total_questions].options[0], "Vincent van Gogh");
    strcpy(game->questions[game->total_questions].options[1], "Pablo Picasso");
    strcpy(game->questions[game->total_questions].options[2], "Leonardo da Vinci");
    strcpy(game->questions[game->total_questions].options[3], "Michelangelo");
    game->questions[game->total_questions].correct_option = 2;
    game->questions[game->total_questions].difficulty = DIFFICULTY_HARD;
    game->total_questions++;

    // Add more hard questions to reach at least 10
    strcpy(game->questions[game->total_questions].question, "What is the largest planet in our solar system?");
    strcpy(game->questions[game->total_questions].options[0], "Earth");
    strcpy(game->questions[game->total_questions].options[1], "Saturn");
    strcpy(game->questions[game->total_questions].options[2], "Jupiter");
    strcpy(game->questions[game->total_questions].options[3], "Neptune");
    game->questions[game->total_questions].correct_option = 2;
    game->questions[game->total_questions].difficulty = DIFFICULTY_HARD;
    game->total_questions++;

    strcpy(game->questions[game->total_questions].question, "Which element has the atomic number 1?");
    strcpy(game->questions[game->total_questions].options[0], "Helium");
    strcpy(game->questions[game->total_questions].options[1], "Hydrogen");
    strcpy(game->questions[game->total_questions].options[2], "Oxygen");
    strcpy(game->questions[game->total_questions].options[3], "Carbon");
    game->questions[game->total_questions].correct_option = 1;
    game->questions[game->total_questions].difficulty = DIFFICULTY_HARD;
    game->total_questions++;

    strcpy(game->questions[game->total_questions].question, "In which year did World War II end?");
    strcpy(game->questions[game->total_questions].options[0], "1943");
    strcpy(game->questions[game->total_questions].options[1], "1945");
    strcpy(game->questions[game->total_questions].options[2], "1947");
    strcpy(game->questions[game->total_questions].options[3], "1950");
    game->questions[game->total_questions].correct_option = 1;
    game->questions[game->total_questions].difficulty = DIFFICULTY_HARD;
    game->total_questions++;
}

void shuffle_questions(Question* questions, int count) {
    for (int i = count - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Question temp = questions[i];
        questions[i] = questions[j];
        questions[j] = temp;
    }
}