#include <SDL2/SDL.h>
#include <math.h>

#define SAMPLE_RATE 44100
#define AMPLITUDE 0.25  // Reduced to prevent clipping when combining waves
#define BUFFER_SIZE 1024
#define MAX_FREQUENCIES 4  // Number of different frequencies to generate

int WIDTH = 900, HEIGHT = 600;

typedef struct {
  float frequencies[MAX_FREQUENCIES];
  float phases[MAX_FREQUENCIES];
  float* individual_waveforms[MAX_FREQUENCIES];
  float* combined_waveform;
} AudioState;

void enter_fullscreen(SDL_Window* window) {
  SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
  SDL_GetWindowSize(window, &WIDTH, &HEIGHT);
}

void generate_sine_wave(float* buffer, int len, float freq, float* phase) {
  for (int i = 0; i < len; i++) {
    buffer[i] = AMPLITUDE * sinf(*phase);
    *phase += 2.0f * M_PI * freq / SAMPLE_RATE;
    if (*phase > 2.0f * M_PI) *phase -= 2.0f * M_PI;
  }
}

void combine_waveforms(AudioState* state, int len) {
  // Clear combined waveform buffer
  memset(state->combined_waveform, 0, len * sizeof(float));
  
  // Generate and add each frequency
  for (int f = 0; f < MAX_FREQUENCIES; f++) {
    if (state->frequencies[f] > 0) {
      generate_sine_wave(state->individual_waveforms[f], len, state->frequencies[f], &state->phases[f]);
      
      // Add to combined waveform
      for (int i = 0; i < len; i++) state->combined_waveform[i] += state->individual_waveforms[f][i];
    }
  }
}

void plot_waveform(SDL_Renderer* renderer, float* waveform, int len, int yOffset, int height, SDL_Color color) {
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
  
  for (int x = 1; x < WIDTH; x++) {
    float idx1 = ((x - 1) * (float)len) / WIDTH;
    float idx2 = (x * (float)len) / WIDTH;
    
    int y1 = yOffset - (int)(waveform[(int)idx1] * ((float)height / 2));
    int y2 = yOffset - (int)(waveform[(int)idx2] * ((float)height / 2));
    
    SDL_RenderDrawLine(renderer, x - 1, y1, x, y2);
  }
}

void audio_callback(void* userdata, Uint8* stream, int len) {
  AudioState* state = (AudioState*)userdata;
  Sint32* output = (Sint32*)stream;
  int samples = len / sizeof(Sint32);
  
  combine_waveforms(state, samples);
  
  // Convert combined waveform to output format
  for (int i = 0; i < samples; i++) {
      output[i] = (Sint32)(state->combined_waveform[i] * 2147483647.0f);
  }
}

int main() {
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
  SDL_Window* window = SDL_CreateWindow("FourPlay", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  enter_fullscreen(window);

  // Initialize audio state
  AudioState state = {
    .frequencies = {440.0f, 554.37f, 659.25f, 880.0f},  // A4, C#5, E5, A5 (A major chord)
    .phases = {0.0f, 0.0f, 0.0f, 0.0f}
  };

  // Allocate waveform buffers
  state.combined_waveform = (float*)calloc(BUFFER_SIZE, sizeof(float));
  for (int i = 0; i < MAX_FREQUENCIES; i++) state.individual_waveforms[i] = (float*)calloc(BUFFER_SIZE, sizeof(float));

  SDL_AudioSpec wanted_spec = {
    .freq = SAMPLE_RATE,
    .format = AUDIO_S32SYS,
    .channels = 1,
    .samples = BUFFER_SIZE,
    .callback = audio_callback,
    .userdata = &state
  };
  
  SDL_AudioSpec obtained_spec;
  SDL_OpenAudio(&wanted_spec, &obtained_spec);
  SDL_PauseAudio(0);

  // Define colors for different waveforms
  SDL_Color colors[5] = {
    {255, 255, 255, 255},  // White for combined
    {255, 0, 0, 255},      // Red
    {0, 255, 0, 255},      // Green
    {0, 0, 255, 255},      // Blue
    {255, 255, 0, 255}     // Yellow
  };

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
    
    // Draw combined waveform in top half
    plot_waveform(renderer, state.combined_waveform, BUFFER_SIZE, HEIGHT/4, HEIGHT/2, colors[0]);
    
    // Draw individual waveforms in bottom half
    int subHeight = HEIGHT/9; // Adjust this for Containing display
    for (int i = 0; i < MAX_FREQUENCIES; i++) plot_waveform(renderer, state.individual_waveforms[i], BUFFER_SIZE, HEIGHT/2 + (i+1)*subHeight, subHeight, colors[i+1]);
    
    SDL_RenderPresent(renderer);
    SDL_Delay(16);
  }

  // Cleanup
  SDL_CloseAudio();
  free(state.combined_waveform);
  for (int i = 0; i < MAX_FREQUENCIES; i++) free(state.individual_waveforms[i]);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  
  return 0;
}
