library(tidyverse)

# load the data
dat <- read_tsv("data/combined_data_labeled.tsv.bz2",
  col_types = cols(
    index = col_integer(),
    elapsed_time = col_double(),
    REF_D_x = col_double(),
    REF_D_y = col_double(),
    REF_D_z = col_double(),
    REF_W_x = col_double(),
    REF_W_y = col_double(),
    REF_W_z = col_double(),
    CAR_W_x = col_double(),
    CAR_W_y = col_double(),
    CAR_W_z = col_double(),
    CAR_D_x = col_double(),
    CAR_D_y = col_double(),
    CAR_D_z = col_double(),
    subj_w = col_factor(),
    subj_d = col_factor(),
    filename = col_factor(),
    condition = col_factor(),
    trial = col_integer(),
    trial_elapsed_time = col_double()
  ))

# first lets create an "experiment id" for each subject combination


l_names <- unique(paste0(dat$subj_w, dat$subj_d))
exp_id_map <- structure(1:length(l_names), names = l_names)

# use exp_id_map to create a new column
dat <- dat %>%
    mutate(
        experiment_id = exp_id_map[paste0(subj_w, subj_d)]
    )


# make data long form

# first split on subject
dat_long <- dat %>%
  pivot_longer(
    c(subj_w, subj_d),
    names_to = "subj_pos",
    values_to = "subj")

# now rename values in subj_pos to remove the subj_ prefix
dat_long <- dat_long %>%
  mutate(subj_pos = str_replace(subj_pos, "subj_", ""))

# now we need to replace CAR_{W,D}_{x,y,z} with CAR_{x,y,z}
# based on the subj_pos column

dat_long <- dat_long %>%
  mutate(
    CAR_x = if_else(subj_pos == "w", CAR_W_x, CAR_D_x),
    CAR_y = if_else(subj_pos == "w", CAR_W_y, CAR_D_y),
    CAR_z = if_else(subj_pos == "w", CAR_W_z, CAR_D_z)
  )



# we will still leave the ref cols in there for now
# we probably should use them for alignment
# but all the CAR_W* and CAR_D* cols are now CAR_* cols
# so they can be removed

dat_long <- dat_long %>%
  select(-starts_with("CAR_W")) %>%
  select(-starts_with("CAR_D"))

# we also no longer need elapsed_time
dat_long <- dat_long %>%
  select(-elapsed_time)


# we can also replace index with a relative index
# this will be useful for plotting
dat_long <- dat_long %>%
  group_by(subj, subj_pos, condition, trial) %>%
  mutate(index = row_number())

head(dat_long)

# let's just get a report of the mean, SD, min and max for each subject's X and Y position

dat_long %>%
  group_by(subj, subj_pos, condition, trial) %>%
  summarise(
    x_mean = mean(CAR_x),
    x_sd = stats::sd(CAR_x),
    x_min = min(CAR_x),
    x_max = max(CAR_x),
    y_mean = mean(CAR_y),
    y_sd = stats::sd(CAR_y),
    y_min = min(CAR_y),
    y_max = max(CAR_y)
  )

# now let's report on the mean, SD, min and max for REF_W_* and REF_D_* across all trials

dat_long %>% ungroup() %>%
  summarise(
    x_w_mean = mean(REF_W_x),
    x_w_sd = stats::sd(REF_W_x),
    x_w_min = min(REF_W_x),
    x_w_max = max(REF_W_x),
    y_w_mean = mean(REF_W_y),
    y_w_sd = stats::sd(REF_W_y),
    y_w_min = min(REF_W_y),
    y_w_max = max(REF_W_y),
    z_w_mean = mean(REF_W_z),
    z_w_sd = stats::sd(REF_W_z),
    z_w_min = min(REF_W_z),
    z_w_max = max(REF_W_z),
    x_d_mean = mean(REF_D_x),
    x_d_sd = stats::sd(REF_D_x),
    x_d_min = min(REF_D_x),
    x_d_max = max(REF_D_x),
    y_d_mean = mean(REF_D_y),
    y_d_sd = stats::sd(REF_D_y),
    y_d_min = min(REF_D_y),
    y_d_max = max(REF_D_y),
    z_d_mean = mean(REF_D_z),
    z_d_sd = stats::sd(REF_D_z),
    z_d_min = min(REF_D_z),
    z_d_max = max(REF_D_z)
  )



# now let's save the data
con <- bzfile("data/combined_data_long.tsv.bz2")
write_tsv(dat_long, con)
