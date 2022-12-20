# this is where we will align the marker 3d data using mousetrap.

library(tidyverse)
library(mousetrap)
library(ggprism)
# library(cowplot)


# load the data
dat <- read_tsv("data/combined_data_long.tsv.bz2",
  col_types = cols(
    index = col_integer(),
    REF_D_x = col_double(),
    REF_D_y = col_double(),
    REF_D_z = col_double(),
    REF_W_x = col_double(),
    REF_W_y = col_double(),
    REF_W_z = col_double(),
    condition = col_factor(),
    trial = col_factor(),
    trial_elapsed_time = col_double(),
    experiment_id = col_factor(),
    subj_pos = col_factor(),
    subj = col_factor(),
    CAR_x = col_double(),
    CAR_y = col_double(),
    CAR_z = col_double(),
    filename = col_factor()
  ))

# add a unique trial ID which is a combination of experiment_id, subj, condition, trial
dat <- dat %>%
  mutate(
    mt_id = paste0(experiment_id, subj, condition, trial)
  )

# now we can remove every column except the ones used for mousetrap
dat <- dat %>%
  select(
    mt_id,
    index,
    trial_elapsed_time,
    CAR_x,
    CAR_y,
    CAR_z,
    trial,
    subj,
    subj_pos,
    condition,
    experiment_id
  )
# we can also remove all trial "1" data since that is the practice trial
dat <- dat %>%
  filter(trial != "1")

# timestamps seem to be required to be an integer, right now they are doubles
# number of seconds with 5 decimal places
# dat <- dat %>%
#  mutate(trial_elapsed_time = as.integer(trial_elapsed_time * 100000))

levels(dat$experiment_id) <- paste("Subject Pair", levels(dat$experiment_id))

levels(dat$trial) <- paste("Trial", as.integer(levels(dat$trial)) - 1)

levels(dat$condition) <- c("Sync", "Task", "No Sonification")

dat_mt <- mt_import_long(dat,
  xpos_label = "CAR_x",
  ypos_label = "CAR_y",
  zpos_label = "CAR_z",
  timestamps_label = "trial_elapsed_time",
  mt_id_label = "mt_id",
  mt_seq_label = "index")

dat_mt_aligned <- mt_align_start(dat_mt, start = c(0, 0, 0))

# dat_mt_aligned_normalized <- mt_length_normalize(
#     dat_mt_aligned,
#     dimensions = c("xpos", "ypos", "zpos"))

# ln_trajectories

# mt_plot(dat_mt_aligned_normalized,
#   use = "ln_trajectories",
#   x = "timestamps",
#   color = "experiment_id")
grid::current.viewport()
# plt <- mt_plot(dat_mt_aligned,
#   use = "trajectories",
#   x = "timestamps",
#   color = "experiment_id")

# dat_mt_aligned <- mt_measures(dat_mt_aligned)

dat_mt_scaled <- mt_scale_trajectories(
    dat_mt_aligned,
    within_trajectory = TRUE,
    var_names = c("ypos"), prefix = "z_")

trajectories <- mt_reshape(dat_mt_scaled,
        use2_variables = c("trial", "experiment_id", "subj", "condition"),
        aggregate = FALSE, subset = condition == "No Sonification")


plt <- ggplot(
  trajectories,
  aes(x = timestamps, y = z_ypos, color = subj)) +
  geom_path(alpha = 0.6) +
  scale_color_prism("floral") +
  guides(y = "prism_offset_minor") +
  theme_prism(base_size = 16) +
  theme(legend.position = "none") +
  labs(
    y = "Standardized Y Position",
    x = "Time (s)",
    title = "Subject Pair Trajectories: No Sonification") +
  facet_grid(
    rows = vars(trial),
    cols = vars(experiment_id),
    scales = "free_x",
    space = "free_x"
  )

ggsave(
  "docs/figures/mt_trajectories_no_sonification.png",
  plt,
  width = 16,
  height = 9,
  units = "in",
  dpi = 300
)



trajectories <- mt_reshape(dat_mt_scaled,
        use2_variables = c("trial", "experiment_id", "subj", "condition"),
        aggregate = FALSE, subset = condition == "Sync")


plt <- ggplot(
  trajectories,
  aes(x = timestamps, y = z_ypos, color = subj)) +
  geom_path(alpha = 0.6) +
  scale_color_prism("floral") +
  guides(y = "prism_offset_minor") +
  theme_prism(base_size = 16) +
  theme(legend.position = "none") +
  labs(
    y = "Standardized Y Position",
    x = "Time (s)",
    title = "Subject Pair Trajectories: Sync Sonification") +
  facet_grid(
    rows = vars(trial),
    cols = vars(experiment_id),
    scales = "free_x",
    space = "free_x"
  )

ggsave(
  "docs/figures/mt_trajectories_sync_sonification.png",
  plt,
  width = 16,
  height = 9,
  units = "in",
  dpi = 300
)


trajectories <- mt_reshape(dat_mt_scaled,
        use2_variables = c("trial", "experiment_id", "subj", "condition"),
        aggregate = FALSE, subset = condition == "Task")


plt <- ggplot(
  trajectories,
  aes(x = timestamps, y = z_ypos, color = subj)) +
  geom_path(alpha = 0.6) +
  scale_color_prism("floral") +
  guides(y = "prism_offset_minor") +
  theme_prism(base_size = 16) +
  theme(legend.position = "none") +
  labs(
    y = "Standardized Y Position",
    x = "Time (s)",
    title = "Subject Pair Trajectories: Task Sonification") +
  facet_grid(
    rows = vars(trial),
    cols = vars(experiment_id),
    scales = "free_x",
    space = "free_x"
  )

ggsave(
  "docs/figures/mt_trajectories_task_sonification.png",
  plt,
  width = 16,
  height = 9,
  units = "in",
  dpi = 300
)


# now let's save the standardized trajectories

trajectories <- mt_reshape(dat_mt_scaled,
        use2_variables = c("trial", "experiment_id", "subj", "condition"),
        aggregate = FALSE)

# we can remove xpos and zpos, since we are only analyzing one axis
# we also do not need the mt_id, but we will keep the mt_seq (index)
trajectories <- trajectories %>% select(-xpos, -zpos, -mt_id)


save(
  trajectories,
  file = "data/standardized_trajectories.Rda.bz2",
  compress = "bzip2")

# also save as a bzipped tsv for compatibility with other tools
con <- bzfile("data/standardized_trajectories.tsv.bz2")
write_tsv(trajectories, con)
