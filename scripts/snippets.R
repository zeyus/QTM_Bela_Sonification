# Data
trajectories_file <- "../data/standardized_trajectories.Rda.bz2"
cache_dir <- ".cache"
capture_freq <- 300

r <- getOption("repos")
r["CRAN"] <- "https://mirrors.dotsrc.org/cran/" # Denmark
options(repos = r)
rm(r)

# use this function to load packages to save manual installation
use_package <- function(p) {
    if (!is.element(p, installed.packages()[, 1]))
        install.packages(p, dep = TRUE)
    require(p, character.only = TRUE)
}
use_package("conflicted")
use_package("tidyverse")
use_package("easystats")
use_package("ggprism")
use_package("gsignal")
# use_package("circular")
# use_package("Directional")
use_package("pracma")
# use_package("grid")
# use_package("gridExtra")
# use_package("ggrepel")
use_package("colorspace")
# use_package("lme4")
# use_package("lmerTest")
# use_package("rstanarm")
# use_package("sjPlot")
# use_package("sjlabelled")
# use_package("sjmisc")
# use_package("broom.mixed")
# use_package("gtsummary")
# use_package("DiagrammeR")
# use_package("DiagrammeRsvg")
# use_package("rsvg")
use_package("zoo")
use_package("dtw")
use_package("unigd")
# use_package("ggthemes")
use_package("astsa")

colorblind_friendly_pal <- c("#0074e7", "#e7298a", "#8bcf00")
# darker version
colorblind_friendly_pal_dark <- darken(colorblind_friendly_pal, 0.5)
# make it slightly less intense
colorblind_friendly_pal <- lighten(colorblind_friendly_pal, 0.25)
options(ggplot2.discrete.colour=colorblind_friendly_pal)
options(ggplot2.discrete.fill=colorblind_friendly_pal)

# this is slow, so check for a cached version
trajectories_with_hilbert <-
    paste0(cache_dir, "/trajectories_with_hilbert.rds.bz2")
# if the cached version exists, use it!
if (file.exists(trajectories_with_hilbert)) {
    angular_trajectories <- readRDS(trajectories_with_hilbert)
    rm(trajectories_with_hilbert)
}


# let's just grab two subjects
sample_exp <- angular_trajectories %>%
  dplyr::filter(experiment_id == "Subject Pair 3", condition == "Sync", trial == "Trial 1")

# get the subject ids
subj_ids <- unique(sample_exp$subj)

# get the data for each subject
subj_a <- sample_exp %>%
  dplyr::filter(subj == subj_ids[1])

subj_b <- sample_exp %>%
  dplyr::filter(subj == subj_ids[2])

# get the y position
y_pos_a <- subj_a$z_ypos
y_pos_b <- subj_b$z_ypos

# decimate the data
#y_pos_a <- y_pos_a[seq(1, length(y_pos_a), 10)]
#y_pos_b <- y_pos_b[seq(1, length(y_pos_b), 10)]

# y_pos_a_wnd <- window(y_pos_a, start = 30 * 80, end = 30 * 84)
# y_pos_b_wnd <- window(y_pos_b, start = 30 * 80, end = 30 * 84)

pos_mat <- matrix(
  c(y_pos_a, y_pos_b),
  ncol = 2,
  nrow=length(y_pos_a),
  byrow = TRUE,
  dimnames = list(
    1:length(y_pos_a),
    c("x", "y")
  )
)
# now let's try a windowed cross correlation between the example pair of trajectories
sample_freq <- capture_freq
window_size <- sample_freq * 5
step_size <- floor(window_size / 2)
lag_max <- floor(window_size / 2 - 1)
lag_range <- seq.int(-lag_max, lag_max, 1)

windowed_xcorr <- zoo::rollapply(
  pos_mat,
  width = window_size,
  by = step_size,
  by.column = FALSE,
  FUN = \(x) {
    xc <- stats::ccf(
      x[, 1],
      x[, 2],
      plot = FALSE,
      lag.max = lag_max
    )

    return(xc)
  }
)


x_cor_res_acf <- windowed_xcorr[,1] %>% unlist()
x_cor_res_lag <- windowed_xcorr[,4] %>% unlist()

n_windows <- length(windowed_xcorr[,1])
n_lags <- length(windowed_xcorr[[1]])

xcorr_tbl <- tibble::tibble(
  x_cor_res_acf = x_cor_res_acf,
  x_cor_res_lag = x_cor_res_lag,
  window_id = rep(1:n_windows, each=n_lags),
  lag_id = rep(lag_range, n_windows)
)

# plot a heatmap
xcorr_tbl %>%
  ggplot(aes(x = lag_id / sample_freq, y = window_id, fill = x_cor_res_acf)) +
  geom_tile() +
  scale_fill_gradient2(low = "blue", mid = "white", high = "red", midpoint = 0) +
  # mark max correlation for each window
  geom_point(
    data = xcorr_tbl %>%
      group_by(window_id) %>%
      summarize(
        max_x_cor_res_acf = max(x_cor_res_acf),
        max_x_cor_res_lag = lag_id[x_cor_res_acf == max_x_cor_res_acf]
      ),
    mapping = aes(
      x = max_x_cor_res_lag / sample_freq,
      y = window_id),
    inherit.aes = FALSE,
    color = "black",
    size = 1
  ) +
  theme_bw() +
  theme(
    axis.text.x = element_text(angle = 90, hjust = 1, vjust = 0.5),
    axis.text.y = element_text(angle = 0, hjust = 1, vjust = 0.5)
  ) +
  labs(
    x = "Lag (s)",
    y = "Window",
    fill = "Cross Correlation"
  )

# plot the two trajectories
ggplot() +
  geom_line(
    data = subj_a,
    mapping = aes(x = mt_seq / sample_freq, y = z_ypos),
    color = "blue"
  ) +
  geom_line(
    data = subj_b,
    mapping = aes(x = mt_seq / sample_freq, y = z_ypos),
    color = "red"
  ) +
  theme_bw() +
  theme(
    axis.text.x = element_text(angle = 90, hjust = 1, vjust = 0.5),
    axis.text.y = element_text(angle = 0, hjust = 1, vjust = 0.5)
  ) +
  labs(
    x = "Time (s)",
    y = "Y Position"
  )

# attempt to turn trajectories at peaks into monotonous trajectories
threshold_value <- 1.2
threshold_time_difference <- capture_freq * 0.5

mono_trajectories <- angular_trajectories %>%
  group_by(experiment_id, condition, trial, subj) %>%
  mutate(
    peak_locs = mt_seq %in% ((length(z_ypos) + 1) - gsignal::findpeaks(
      rev(z_ypos),
      MinPeakHeight = threshold_value,
      MinPeakDistance = threshold_time_difference,
      DoubleSided = TRUE)$loc),
    z_ypos_diff = c(0, diff(z_ypos))
  )
# now we will reverse the direction for all z_ypos values between each peak
# with a negative z_ypos and the subsequent peak with a positive z_ypos value
mono_trajectories <- mono_trajectories %>%
  group_by(experiment_id, condition, trial, subj) %>%
  mutate(
    reverse = ifelse(z_ypos < 0 & peak_locs, 1, NA)
  ) %>%
  mutate(
    reverse = ifelse(is.na(reverse), ifelse(z_ypos > 0 & peak_locs, 0, NA), reverse)
  )
# now we need to fill all NA mono_trajectories$reverse with the previous value
# we should make sure the first value in the series is 0
mono_trajectories <- mono_trajectories %>%
  group_by(experiment_id, condition, trial, subj) %>%
  mutate(
    reverse = ifelse(mt_seq == 1 & is.na(reverse), 0, reverse)
  )

# we  only want to reverse IF the current value is 1 AND previous value was 0
# AND the next value is 0 (otherwise there are two subsequent peaks)
# but there can be many NA values in between
mono_trajectories <- mono_trajectories %>%
  group_by(experiment_id, condition, trial, subj) %>%
  mutate(
    forward = reverse,
    backwards = reverse
  )

mono_trajectories <- mono_trajectories %>%
  tidyr::fill(forward) %>%
  tidyr::fill(backwards, .direction = "up")

mono_trajectories <- mono_trajectories %>%
  mutate(
    reverse = ifelse(
      reverse == 1 &
      dplyr::lag(forward) == 0 &
      dplyr::lead(backwards) == 0, 1, 0)
  )

mono_trajectories <- mono_trajectories %>%
  tidyr::fill(reverse)

mono_trajectories <- mono_trajectories %>%
  mutate(
    z_ypos_diff = ifelse(reverse == 1, -z_ypos_diff, z_ypos_diff)
  ) %>% 
  mutate(
    z_ypos_cum = cumsum(z_ypos_diff)
  )

# mono_trajectories %>%
#   dplyr::filter(
#     condition == "No Sonification",
#     experiment_id == "Subject Pair 3",
#     trial == "Trial 1") %>% pull(subj) %>% unique()

# mono_trajectories %>%
#   dplyr::filter(
#     condition == "No Sonification",
#     experiment_id == "Subject Pair 3",
#     subj == "ODUPC",
#     trial == "Trial 1") %>% pull(z_ypos) %>%
#     gsignal::findpeaks(
#       MinPeakHeight = threshold_value,
#       MinPeakDistance = threshold_time_difference,
#       DoubleSided = TRUE)

mono_trajectories %>%
  dplyr::filter(condition == "No Sonification", experiment_id == "Subject Pair 3", trial == "Trial 2") %>%
  ggplot(aes(x = mt_seq / capture_freq, y = z_ypos)) +
  geom_line(aes(color = subj)) +
  geom_line(aes(y = z_ypos_cum, color = subj), linetype = 2) +

  theme_hc(style = "darkunica") +
  theme(
    axis.text.x = element_text(color = darken("white", 0.4)),
    axis.text.y = element_text(color = darken("white", 0.4)),
    legend.position = "none"
  ) +
  labs(x = "Time (seconds)", y = "Z Position (meters)")


########## TESTING dtw ##########

# angular_trajectories %>%
#  group_by(experiment_id) %>%
#  mutate(group_dummy = as.character(x = factor(x = subj, labels = seq_len(length.out = n_distinct(x = subj))))) %>%
#  ungroup() %>% pull(group_dummy) %>% unique()

# ugd(width=1600, height=1200)
# angular_trajectories %>%
#   dplyr::filter(condition == "Sync", experiment_id %in% c("Subject Pair 4", "Subject Pair 4")) %>%
#   ggplot(aes(x = mt_seq / 300, y = z_ypos, color = subj)) +
#     geom_path() +
#     theme_hc(style = "darkunica") +
#     theme(
#       strip.background = element_rect(fill="#00000000"),
#       strip.text = element_text(color="white"),
#     ) +
#     theme(legend.position = "none") +
#     labs(
#       y = "Standardized Y Position",
#       x = "Time (s)",
#       title = "Subject Pair Trajectories: Sync Sonification") +
#     facet_grid(
#       rows = vars(trial),
#       cols = vars(experiment_id),
#       scales = "free_x",
#       space = "free_x"
#     )
# ugd_save("figures/tmp_pres.png", width=1600, height=1200, zoom = 3)
# dev.off()

# # let's just grab two subjects
# sample_exp <- angular_trajectories %>%
#   dplyr::filter(experiment_id == "Subject Pair 3", condition == "Sync", trial == "Trial 1")

# # get the subject ids
# subj_ids <- unique(sample_exp$subj)

# # get the data for each subject
# subj_a <- sample_exp %>%
#   dplyr::filter(subj == subj_ids[1])

# subj_b <- sample_exp %>%
#   dplyr::filter(subj == subj_ids[2])

# # get the y position
# y_pos_a <- subj_a$z_ypos
# y_pos_b <- subj_b$z_ypos
# ugd(width=1600, height=1200)
# ggplot() +
#   geom_line(aes(x = subj_a$mt_seq / 300, y = subj_a$z_ypos), linetype = 3, linewidth = 1.5, color = colorblind_friendly_pal[1]) +
#   geom_line(aes(x = subj_b$mt_seq / 300, y = subj_b$z_ypos), linetype = 3, linewidth = 1.5, color = colorblind_friendly_pal[2]) +
#   theme_hc(style = "darkunica") +
#   theme(legend.position = "none",
#   axis.text = element_text(size = rel(2)),
#   axis.title = element_text(size = rel(2))) +
#   labs(x = "Time (s)", y = "Standardized X-Position")
# ugd_save("figures/a_b_raw.png", width=1600, height=1200, zoom=2)
# dev.off()
# # alignments_default <- dtw::dtw(y_pos_a, y_pos_b, keep.internals = TRUE)
# # plot(alignments_default, type="threeway")
# # dtwPlotDensity(alignments_default)

# # let's downsample the data because dtw crashes otherwise

# bandpass <- function(x, low, high, fs) {
#   # bandpass filter
#   # x is the signal
#   # low is the low cutoff frequency
#   # high is the high cutoff frequency
#   # fs is the sampling frequency
#   # returns the filtered signal
#   nyq <- fs / 2
#   b <- gsignal::butter(3, c(low, high) / nyq, "pass", output = "Sos")
#   gsignal::filter(b, x)
# }

# fft_a <- fft(y_pos_a)
# spec_res <- seewave::spec(y_pos_a, f = 300, wl = length(y_pos_a) / 3, fftw = TRUE, scaled = FALSE, norm = TRUE, flim = c(0, 0.5/1000))

# # print the peak frequencies
# spec_res[which.max(spec_res[, 2]), ] * 1000

# spec.ar(y_pos_a)
# spec.ar(y_pos_b)
# plot(fft_a)
# plot(fft(y_pos_b))


# y_pos_a <- bandpass(subj_a$z_ypos, 0.001, 0.5, 300)
# y_pos_b <- bandpass(subj_b$z_ypos, 0.001, 0.5, 300)
# ugd(width=1600, height=1200)
# plot(y_pos_a, type = "l")
# ugd_save("figures/subject_a_bandpass.png", width=1600, height=1200)
# dev.off()
# plot(subj_a$z_ypos, type = "l")
# plot(y_pos_b, type = "l")
# plot(subj_b$z_ypos, type = "l")


# # alignments fast ?
# alignments_quick_hf <- dtw::dtw(
#   y_pos_a_ds,
#   y_pos_b_ds,
#   keep.internals = FALSE,
#   distance.only = TRUE,
#   window.type = "sakoechiba",
#   window.size = 30 * 3) # 3 second window



# y_pos_a_ds <- y_pos_a[seq(1, length(y_pos_a), 10)]
# y_pos_b_ds <- y_pos_b[seq(1, length(y_pos_b), 10)]

# alignments_quick_lf <- dtw::dtw(
#   y_pos_a_ds,
#   y_pos_b_ds,
#   keep.internals = FALSE,
#   distance.only = TRUE,
#   window.type = "sakoechiba",
#   window.size = capture_freq / 10 * 2) # 2 second window


# alignments_quick_hf
# alignments_quick_lf


# alignments_default <- dtw::dtw(y_pos_a_ds, y_pos_b_ds, keep.internals = TRUE)
# ugd(width=1600, height=1200)
# plot(alignments_default, type="twoway")
# ugd_save("figures/subject_a_bandpass_dtw_ds.png", width=1600, height=1200, zoom = 3)
# dev.off()
# # dtwPlotDensity(alignments_default, normalize = TRUE)
# # dtwPlotDensity(alignments_default, normalize = FALSE)

# alignments_windowed <- dtw::dtw(y_pos_a_ds, y_pos_b_ds, keep.internals = TRUE)
# ugd(width=1600, height=1200)
# dtwPlotDensity(alignments_windowed, normalize = FALSE)
# ugd_save("figures/subject_a_bandpass_dtw_ds_windowed.png", width=1600, height=1200, zoom = 3)
# dev.off()

# ugd(width=1600, height=1200)
# dtwPlotDensity(alignments_windowed, normalize = TRUE)
# ugd_save("figures/subject_a_bandpass_dtw_ds_windowed_norm.png", width=1600, height=1200)
# dev.off()



# alignments_default <- dtw::dtw(y_pos_a_ds, y_pos_b_ds, keep.internals = TRUE)
# ugd(width=1600, height=1200)
# plot(alignments_default, type="twoway")
# ugd_save("figures/subject_a_dtw_ds.png", width=1600, height=1200)
# dev.off()

# alignments_windowed <- dtw::dtw(y_pos_a_ds, y_pos_b_ds, keep.internals = TRUE,window=sakoeChibaWindow, window.size = 3 * 10)
# ugd(width=1600, height=1200)
# dtwPlotDensity(alignments_windowed, normalize = FALSE)
# ugd_save("figures/subject_a_dtw_ds_windowed.png", width=1600, height=1200)
# dev.off()

# ugd(width=1600, height=1200)
# dtwPlotDensity(alignments_windowed, normalize = TRUE)
# ugd_save("figures/subject_a_dtw_ds_windowed_norm.png", width=1600, height=1200)
# dev.off()



########## /TESTING dtw ##########


# TEMP
# plot the mean and standard deviation for distance delta by condition
trajectories_angles_pairwise$condition <- factor(trajectories_angles_pairwise$condition, levels = c("No Sonification", "Task", "Sync"))
ugd(width=1600, height=1200)
trajectories_angles_pairwise %>%
    group_by(condition, trial) %>%
    summarise(delta_y = mean(delta_y), .groups="drop") %>%
    ggplot(aes(
        x = condition,
        y = delta_y,
        fill = condition)) +
    geom_boxplot(show.legend = FALSE, color = darken("white", 0.2)) +
    stat_summary(
      aes(
        color = stage(condition, after_scale = darken(color, 0.70))),
      fun = "mean",
      geom = "point",
      shape = 8,
      size = 2,
      show.legend = FALSE) +
    guides(y = "prism_offset_minor") +
    theme_hc(style = "darkunica") +
    theme(
      axis.text.x = element_text(color = darken("white", 0.4)),
      axis.text.y = element_text(color = darken("white", 0.4)),
      legend.position = "none"
    ) +
    labs(
        x = "Condition",
        y = "Mean pairwise position delta")

ugd_save("figures/dist_summary.png", width=1600, height=1200, zoom = 3)
dev.off()
# /TEMP


spec_res <- seewave::spec(angular_trajectories$z_ypos, plot = FALSE, f = 300, wl = length(angular_trajectories$z_ypos) / 3, fftw = TRUE, scaled = FALSE, norm = TRUE, flim = c(0, 1/1000))

# print the peak frequencies
spec_res <- as_tibble(spec_res)
spec_res$x <- spec_res$x * 1000 # kHz to Hz
max(spec_res$x)
ugd(width=1600, height=1200)
spec_res %>%
  dplyr::filter(x <= 0.21) %>%
  ggplot(aes(x = x, y = y)) +
  geom_line(color = colorblind_friendly_pal[2]) +
  theme_hc(style = "darkunica") +
  labs(
    title = "Spectral Analysis",
    x = "Frequency (Hz)",
    y = "Power"
  ) +
  theme(
    axis.text.x = element_text(color = darken("white", 0.4)),
    axis.text.y = element_text(color = darken("white", 0.4))
  )
ugd_save("figures/spec_summary.png", width=1600, height=1200, zoom = 3)
dev.off()


trajectories_angles_pairwise %>%
    dplyr::filter(
        condition == "Sync",
        experiment_id == "Subject Pair 3",
        trial == "Trial 1",
        mt_seq > 26.15 * 300 &
        mt_seq < 27 * 300) %>%
    head()

## This is excluded as it was a plot showing all the subject pairs
## and conditions, but it was too crowded to be useful
ugd(width=1600, height=1200)
trajectories_angles_pairwise %>%
  dplyr::filter(condition == "Sync", experiment_id == "Subject Pair 3", trial == "Trial 1") %>%
ggplot(aes(x = mt_seq / capture_freq, y = instantaneous_phase_angle_diff_c)) +
  geom_line(color = colorblind_friendly_pal[2]) +
  theme_hc(style = "darkunica") +
      theme(
      axis.text.x = element_text(color = darken("white", 0.4)),
      axis.text.y = element_text(color = darken("white", 0.4)),
      legend.position = "none"
  ) +
  labs(x = "Time (seconds)", y = "Instantaneous Phase Angle Difference (degrees)")
ugd_save("figures/phase_angle_example.png", width=1600, height=1200, zoom = 3)
dev.off()









# get the sum of the data points for each time point across a condition
multitrajectory <- function(data, condition, start, finish) {
    x <- data %>% 
        dplyr::filter(
            condition == condition,
            mt_seq >= start * capture_freq &
            mt_seq <= finish * capture_freq &
            experiment_id != "Subject Pair 2") %>% 
        group_by(mt_seq) %>% 
        summarise(x = sum(z_ypos)) %>% 
        ungroup() %>%
        pull(x)
    x
}

Fs <- 300
step <- 1             # one spectral slice every 1/300th of a second
window <- 20*Fs  # 100 ms data window
fftn <- 2^(ceiling(log2(abs(window)))+2)

# plot the spectra
lower_freq <- 0
upper_freq <- 0.5

data <- multitrajectory(angular_trajectories, "No Sonification", 0, 90)

spec_trajectory = signal::specgram(data, n = fftn, Fs = Fs, window = window, overlap = window-step)

freq_idx <- spec_trajectory$f >= lower_freq & spec_trajectory$f <= upper_freq
time_values <- spec_trajectory$t

S <- abs(spec_trajectory$S[freq_idx,])
S <- S/max(S)         # normalize magnitude so that max is 0 dB.
row.names(S) <- spec_trajectory$f[freq_idx]
data <- reshape2::melt(t(10*log10(S)), c("x", "y"), value.name = "z")
data$tone <- "No"

ugd(width=1600, height=1200)
ggplot(data = data, mapping = aes(x = x / 300, y = y, fill = z)) +
  geom_raster() +
  scale_fill_gradientn(colors = hcl.colors(5)) +
  guides(y = "prism_offset_minor") +
  theme_prism(base_size = 16) +
  labs(x = "Time (s)", y = "Frequency (Hz)", fill = "Magnitude (dB)") +
  facet_wrap(~tone, nrow = 1)
ugd_save("figures/spec_sync.png", width=1600, height=1200, zoom = 3)
dev.off()


# grouped_data <- trajectories_angles_pairwise %>%
#   group_by(condition, experiment_id) %>%
#   summarize(mean_delta_y = mean(delta_y),
#             sd_delta_y = stats::sd(delta_y))

# result_tibble <- grouped_data %>%
#   group_by(condition) %>%
#   summarize(mean_delta_y = mean(mean_delta_y),
#             sd_delta_y = mean(sd_delta_y))
# # box plot (geom_boxplot) the mean pairwise position delta by experiment, and condition
# result_tibble %>%
#     ggplot(aes(
#         x = condition,
#         y = mean_delta_y,
#         fill = condition)) +
#     geom_boxplot(show.legend = FALSE) +
#     stat_summary(
#       aes(
#         color = stage(condition, after_scale = darken(color, 0.70))),
#       fun = "mean",
#       geom = "point",
#       shape = 8,
#       size = 2,
#       show.legend = FALSE) +
#     guides(y = "prism_offset_minor") +
#     theme_prism(base_size = 16) +
#     labs(
#         x = "Condition",
#         y = "Mean pairwise position delta")
# outlier.size = 0.2,
#         outlier.fill = NA,
#         outlier.shape = 3,
#         outlier.stroke = 0.2,
#         outlier.alpha=0.5