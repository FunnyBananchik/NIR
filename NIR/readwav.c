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
    for (size_t i = 0; i < k; i = i + 80) {
      sr = 0;
      for (size_t j = i; j < i + 80; j++) {
        sr = sr + buf[j] * buf[j];
      }
      if (sr / 80.0 > limit * limit)
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
    int16_t KK = (int)(0.5 + ((k * fg) / 8000));
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

void bpf(int16_t *buf, size_t k, int16_t *real, int16_t *imag, size_t mn) {
  for (size_t i = 0; i < k; i++) {
    real[i] = 0;
    imag[i] = 0;
    for (size_t j = 0; j < k; j++) {
      real[i] = real[i] + buf[j + k * mn] * cos(-2 * M_PI * i * j / k);
      imag[i] = real[i] + buf[j + k * mn] * sin(-2 * M_PI * i * j / k);
    }
  }
}

double arg_f(size_t k, int16_t *real, int16_t *imag) {
  size_t ind = 0;
  double max = sqrt(real[0] * real[0] + imag[0] * imag[0]) / 3600;
  for (size_t i = 1; i < k; i++) {
    if (sqrt(real[i - 1] * real[i - 1] + imag[i - 1] * imag[i - 1]) / 3600 <
        sqrt(real[i] * real[i] + imag[i] * imag[i]) / 3600) {
      max = sqrt(real[i] * real[i] + imag[i] * imag[i]) / 3600;
      ind = i;
    }
  }
  printf("%lf\n", 2 * M_PI * ind / k);
  return (double)(2 * M_PI * ind / k);
}

void vad(int16_t *buf, size_t k) {
  int16_t *real = malloc(k * sizeof(int16_t));
  int16_t *imag = malloc(k * sizeof(int16_t));
  double frame_size = 0.01;
  size_t num_of_frames = (size_t)(k / (frame_size * 8000));
  double energy_primthresh = 40;
  double sf_primthresh = 5;
  double f_primthresh = 185;
  double sr = 0;
  double sum = 0;
  int16_t pr = 1;
  bpf(buf, 1200, real, imag, 0);
  for (size_t i = 0; i < 3600; i++) {
    sr = sr + buf[i] * buf[i];
    sum = sum + sqrt(real[i] * real[i] + imag[i] * imag[i]) / 3600;
    pr = pr * (sqrt(real[i] * real[i] + imag[i] * imag[i]) / 3600);
  }
  double min_e = sr;
  double min_sfm = 10 * log10(pow(pr, 1.0 / 3600) / (sum / 3600.0));
  double min_f = arg_f(3600, real, imag);
  double e = 0;
  double sfm = 0;
  double f = 0;
  double tresh_e = 0;
  double tresh_sfm = 0;
  double tresh_f = 0;
  size_t speech = 0;
  size_t silence = 0;
  size_t count = 0;
  for (size_t i = 0; i < num_of_frames; i++) {
    sr = 0;
    sum = 0;
    pr = 1;
    bpf(buf, 80, real, imag, i);
    for (size_t j = 0; j < 80; j++) {
      sr = sr + buf[j + 80 * i] * buf[j + 80 * i];
      sum = sum + sqrt(real[i] * real[i] + imag[i] * imag[i]);
      pr = pr * sqrt(real[i] * real[i] + imag[i] * imag[i]);
    }
    e = sr;
    sfm = 10 * log10(pow(pr, 1.0 / 80) / (sum / 80));
    f = arg_f(80, real, imag);
    tresh_e = energy_primthresh * log10(min_e);
    tresh_sfm = sf_primthresh;
    tresh_f = f_primthresh;
    count = 0;
    if ((e - min_e) >= tresh_e)
      count++;
    if ((sfm - min_sfm) >= tresh_sfm)
      count++;
    if ((f - min_f) >= tresh_f)
      count++;
    if (count > 1) {
      silence = 0;
      speech++;
    } else {
      min_e = ((silence * min_e) + e) / (silence + 1);
      speech = 0;
      silence++;
    }
    tresh_e = energy_primthresh * log10(min_e);
    if (silence == 15) {
      printf("Silence\n");
      silence = 0;
    } else if (speech == 5) {
      printf("Speech\n");
      speech = 0;
    }
  }
}

void main() {
  // Работа с файлами

  int16_t sample;
  int16_t count;
  size_t k = 0;
  FILE *FWAV;
  FILE *txtfile;
  txtfile = fopen("samples.txt", "w");
  FWAV = popen("ffmpeg -i 1.wav -f s16le -ar 8000 -ac 1 -", "r");
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
  FWAV = popen("ffmpeg -i 1.wav -f s16le -ar 8000 -ac 1 -", "r");
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
