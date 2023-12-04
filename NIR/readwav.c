#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// Пороговый алгоритм

void porogoviy(int16_t *buf, size_t k) {
  int16_t limit;
  int16_t flag = 1;
  int32_t sr = 0;
  while (flag) {
    printf("Enter the limit value of the amplitude \n");
    scanf("%hd", &limit);
    for (size_t i = 0; i < k; i = i + 441) {
      sr = 0;
      for (size_t j = i; j < i + 441; j++) {
        sr = sr + buf[j] * buf[j];
      }
      if (sr / 441.0 > limit * limit)
        printf("Recognized sound on %ld  \n", i);
    }
    printf("Change the limit?  1 - Yes;  0 - No \n");
    scanf("%hd", &flag);
  }
}

// Алгоритм Герцеля

void herzel(int16_t *buf, size_t k) {
  int16_t flag2 = 1;
  while (flag2) {
    double pi = 3.141592654;
    double fg;
    double u0;
    double u1 = 0;
    double u2 = 0;
    double w;
    double magnitude_squared;
    double magnitude;
    double real;
    double imag;
    printf("Enter the  frequency. \n");
    scanf("%le", &fg);
    int16_t KK = (int)(0.5 + ((k * fg) / 44100));
    w = 2 * pi * KK / k;
    for (size_t i = 0; i < k; i++) {
      u0 = 2 * cos(w) * u1 - u2 + buf[i];
      u2 = u1;
      u1 = u0;
    }
    real = (u1 - u2 * cos(w));
    imag = (u2 * sin(w));
    magnitude_squared = real * real + imag * imag;
    magnitude = sqrt(magnitude_squared);
    printf("Amplitude =%12.5f\n", magnitude);
    printf("Change the frequency?  1 - Yes;  0 - No \n");
    scanf("%hd", &flag2);
  }
}

void vad(int16_t *buf, size_t k) {
//Тут будет код
}

void main() {
  // Работа с файлами

  int16_t sample;
  int16_t count;
  size_t k = 0;
  FILE *FWAV;
  FILE *txtfile;
  txtfile = fopen("samples.txt", "w");
  FWAV = popen("ffmpeg -i 1.wav -f s16le -ac 1 -", "r");
  while (1) {
    count = fread(&sample, 2, 1, FWAV);
    if (count == 1) {
      k++;
      fprintf(txtfile, "%ld     %d \n", k, sample);
    } else
      break;
  }
  pclose(FWAV);
  fclose(txtfile);
  int16_t *buf = malloc(k * sizeof(int16_t));
  FWAV = popen("ffmpeg -i 1.wav -f s16le -ac 1 -", "r");
  fread(buf, 2, k, FWAV);
  pclose(FWAV);
  int16_t l;

  // Выбор алгоритмов

  printf("Choose an algorithm.  1 - Porogoviy;  2 - Herzel;  3 - VAD \n");
  scanf("%hd", &l);

  if (l == 1) {
    porogoviy(buf, k);
  } else if (l == 2) {
    herzel(buf, k);
  } else {
    vad(buf, k);
  }
  free(buf);
}
