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
      imag[i] = imag[i] + buf[j + k * mn] * sin(-2 * M_PI * i * j / k);
    }
  }
}

double arg_f(size_t k, int16_t *real, int16_t *imag) {
  size_t i_real = 0;
  size_t i_imag = 0;
  double max_real = real[0];
  double max_imag = imag[0];
  for (size_t i = 1; i < k; i++) {
    if (real[i] > max_real) {
      max_real = real[i];
      i_real = i;
    }
    if (imag[i] > max_imag) {
      max_imag = imag[i];
      i_imag = i;
    }
  }
  if (max_real > max_imag) {
    return i_real * (8000 / k);
  } else {
    return i_imag * (8000 / k);
  }
}

void vad(int16_t *buf, size_t k) {
  int16_t *real = malloc(k * sizeof(int16_t));
  int16_t *imag = malloc(k * sizeof(int16_t));
  FILE *res;
  res = fopen("res.txt", "w");
  double frame_size = 0.01;
  size_t num_of_frames = (size_t)(k / (frame_size * 8000));
  double energy_primthresh = 40;
  double sf_primthresh = 5;
  double f_primthresh = 185;
  double sr = 0;
  double sum = 0;
  int16_t pr = 1;
  double min_e = sqrtl(sr / 5400);
  double min_sfm = -10 * log10f(expf(pr / 5400) / (sum / 5400.0));
  double min_f = arg_f(5400, real, imag);
  double e = 0;
  double sfm = 0;
  double f = 0;
  double tresh_e = 0;
  double tresh_sfm = 0;
  double tresh_f = 0;
  size_t speech = 0;
  size_t silence = 0;
  size_t count = 0;
  tresh_sfm = sf_primthresh;
  tresh_f = f_primthresh;
  for (size_t i = 0; i < num_of_frames; i++) {
    sr = 0;
    sum = 0;
    pr = 1;
    bpf(buf, 80, real, imag, i);
    for (size_t j = 0; j < 80; j++) {
      sr = sr + buf[j + 80 * i] * buf[j + 80 * i];
      sum = sum + sqrt(real[j] * real[j] + imag[j] * imag[j]);
      pr = pr + logf((sqrt(real[j] * real[j] + imag[j] * imag[j])));
    }
    e = sqrtl(sr / 80.0);
    sfm = -10 * log10f(expf(pr / 80.0) / (sum / 80.0));
    f = arg_f(80, real, imag);
    if (i == 0) {
      min_e = e;
      min_sfm = sfm;
      min_f = f;
    } else if (i < 30) {
      min_e = (e > min_e) ? min_e : e;
      min_sfm = (sfm > min_sfm) ? min_sfm : sfm;
      min_f = (f > min_f) ? min_f : f;
    }
    tresh_e = energy_primthresh * log10f(min_e);
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
      silence++;
      min_e = ((silence * min_e) + e) / (silence + 1);
      speech = 0;
    }
    tresh_e = energy_primthresh * log10f(min_e);
    if (silence > 9) {
      fprintf(res, "Silence\n");
    } else if (speech > 4) {
      fprintf(res, "Speech\n");
    }
  }
  fclose(res);
}

void main() {
  // Работа с файлами

  int16_t sample;
  int16_t count;
  size_t k = 0;
  FILE *fwav;
  FILE *txtfile;
  txtfile = fopen("samples.txt", "w");
  fwav = popen("ffmpeg -i 1.wav -f s16le -ar 8000 -ac 1 -", "r");
  while (1) {
    count = fread(&sample, 2, 1, fwav);
    if (count == 1) {
      k++;
      fprintf(txtfile, "%ld     %d \n", k, sample);
    } else
      break;
  }
  pclose(fwav);
  fclose(txtfile);
  int16_t *buf = malloc(k * sizeof(int16_t));
  fwav = popen("ffmpeg -i 1.wav -f s16le -ar 8000 -ac 1 -", "r");
  fread(buf, 2, k, fwav);
  pclose(fwav);
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
