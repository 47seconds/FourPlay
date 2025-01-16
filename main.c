#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_timer.h>
#include <math.h>

#define SAMPLE_RATE 44100
#define AMPLITUDE 0.5
#define FREQUENCY 440.0
#define BUFFER_SIZE 1024

int WIDTH = 900, HEIGHT = 600;

void enter_fullscreen(SDL_Window* window) {
  SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
  SDL_GetWindowSize(window, &WIDTH, &HEIGHT);
}

void generate_sine_wave(float* buffer, int len, float freq) {
  static float phase = 0.0f;
  for (int i = 0; i < len; i++) {
    buffer[i] = AMPLITUDE * sinf(phase);
    phase += 2.0f * M_PI * freq / SAMPLE_RATE;
    if (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;
  }
}

void plot_waveform(SDL_Renderer* renderer, float* waveform, int len) {
  SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
  
  for (int x = 1; x < WIDTH; x++) {
    float idx1 = ((x - 1) * (float)len) / WIDTH;
    float idx2 = (x * (float)len) / WIDTH;
    
    int y1 = HEIGHT / 2 - (int)(waveform[(int)idx1] * ((float)HEIGHT / 2));
    int y2 = HEIGHT / 2 - (int)(waveform[(int)idx2] * ((float)HEIGHT / 2));
    
    SDL_RenderDrawLine(renderer, x - 1, y1, x, y2);
  }
}

void audio_callback(void* userdata, Uint8* stream, int len) {
    float* waveform = (float*)userdata;
    Sint32* output = (Sint32*)stream;
    int samples = len / sizeof(Sint32);
    
    generate_sine_wave(waveform, samples, FREQUENCY);
    
    // Convert float samples to Sint32
    for (int i = 0; i < samples; i++) {
        output[i] = (Sint32)(waveform[i] * 2147483647.0f); // Convert to full 32-bit range
    }
}

int main() {
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

  SDL_Window* window = SDL_CreateWindow("FourPlay", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  enter_fullscreen(window);

  float* waveform = (float*)calloc(BUFFER_SIZE, sizeof(float));

  SDL_AudioSpec wanted_spec = {
    .freq = SAMPLE_RATE,
    .format = AUDIO_S32SYS,
    .channels = 1,
    .samples = BUFFER_SIZE,
    .callback = audio_callback,
    .userdata = waveform
  };
  
  SDL_AudioSpec obtained_spec;
  SDL_OpenAudio(&wanted_spec, &obtained_spec); 
  SDL_PauseAudio(0);

  int running = 1;
  int pause = 0;
  SDL_Event event;
  
  while(running) {
    while(SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) running = 0;
      else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) running = 0;
      else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
        pause = 1 - pause;
        SDL_PauseAudio(pause);
      }
    }

    if (pause) {
      SDL_Delay(100);
      continue;
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    
    plot_waveform(renderer, waveform, BUFFER_SIZE);
    SDL_RenderPresent(renderer);
    SDL_Delay(16);
  }

  SDL_CloseAudio();
  free(waveform);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  
  return 0;
}
