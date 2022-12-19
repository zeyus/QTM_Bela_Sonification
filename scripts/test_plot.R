library(tidyverse)
library(ggthemes)

# Load data
dat <- read_tsv("data/combined_data_labeled.tsv.bz2")

# Plot
dat %>%
  filter(trial == 2 & condition == "y", filename == "data/data_a_b.tsv.bz2", trial_elapsed_time > 60) %>%
  ggplot(aes(x = trial_elapsed_time)) +
  # geom_smooth(aes(y = CAR_W_y), color = "#00C3FFFF", span = 0.005, method = "loess", se = FALSE) +
  # geom_smooth(aes(y = CAR_D_y), color = "#FF00FB99", span = 0.005, method = "loess", se = FALSE, alpha = 0.5) +
  geom_line(aes(y = CAR_W_y), color = "#00C3FFFF") +
  geom_line(aes(y = CAR_D_y), color = "#FF00FB99") +
  theme_hc(style = "darkunica") +
  labs(x = "Time (s)", y = "CAR Y-pos")
