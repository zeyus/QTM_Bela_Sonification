# this script removes trials that were invalidated for various reasons
# e.g. sonification stopped, participants dropped the cars, etc.
library(tidyverse)

data_file <- "data/combined_data.tsv.bz2"

# read in the data

dat <- read_tsv(data_file, col_types = cols(
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
  type = col_character(),
  event_label = col_character(),
  elapsed_time_event = col_double(),
  filename = col_character(),
  subj_w = col_character(),
  subj_d = col_character()
))

# now first we need to get total number of each event type

event_counts <- dat %>%
    group_by(event_label) %>%
    summarise(count = n())


print(event_counts)

# Event labels:
# TRIAL_START = 's',
# TRIAL_END = 'e',
# EXPERIMENT_START = 'S',
# EXPERIMENT_END = 'E',

# Condition labels
# NO_SONIFICATION = 'n',
# TASK_SONIFICATION = 't',
# SYNC_SONIFICATION = 'y',

# the code had a bug that caused the labels to include occasionally some additional data
# so let's remove the invalid data

dat <- dat %>%
    mutate(event_label = str_replace(event_label, "\xe0W", ""))

event_counts <- dat %>%
    group_by(event_label) %>%
    summarise(count = n())

print(event_counts)

# so that's cleaned up the events, but there's still an anomoly
# "te" is not a label, so let's visualize the event positions (in terms of time)

dat %>%
    filter(!is.na(event_label)) %>%
    ggplot(aes(x = elapsed_time_event, y = as.factor(event_label), color = event_label)) +
    geom_point() +
    facet_wrap(~filename, ncol = 2)

# ok it seems the "te" events are only occurring after the "t" events
# and it corresponds interestingly to the trial start...let's confirm the timestamps

dat %>%
    filter(event_label == "te" | event_label == "s") %>%
    select(event_label, elapsed_time) %>%
    head(n = 10)

# that looks it confirms the theory, to be sure let's make sure there are
# exactly that all "te" events have the same elapsed_time as an "s" event

te_elapsed_times <- dat %>%
    filter(event_label == "te") %>%
    select(index, elapsed_time, event_label, filename)

print(te_elapsed_times)

#  now let's join the data to itself to get the "s" events that correspond to the "te" events

matching_s_events <- dat %>%
    filter(event_label == "s") %>%
    inner_join(te_elapsed_times, by = c("elapsed_time", "filename"), suffix = c("", "_te")) %>%
    select(index_te, event_label, event_label_te, elapsed_time, filename)

print(matching_s_events)


# good, it could be that the elapsed time is off by a few milliseconds
# so let's find the "s" events that are within 10ms of the "te" events

te_elapsed_times <- dat %>%
    filter(event_label == "te") %>%
    select(index, elapsed_time, event_label, filename)

matching_s_events <- dat %>%
    filter(event_label == "s") %>%
    inner_join(te_elapsed_times, by = c("filename"), suffix = c("", "_te")) %>%
    mutate(elapsed_time_diff = abs(elapsed_time - elapsed_time_te)) %>%
    filter(abs(elapsed_time - elapsed_time_te) < 0.01) %>%
    select(index, index_te, event_label, event_label_te, elapsed_time, filename, elapsed_time_diff)

matching_s_events

# problem solved. so these events were definitely invalid, accidental "double" sends
# now we can just set all the "te" events to NA

dat <- dat %>% mutate(event_label = ifelse(event_label == "te", NA, event_label))

# check we only have 0 "te" events left
dat %>%
    filter(event_label == "te") %>%
    head(n = 10)

# now lets make sure the event counts are the same as before

event_counts_cleaned <- dat %>%
    group_by(event_label) %>%
    summarise(count = n())

# check that the counts are the same
print(event_counts_cleaned)
print(event_counts)

# ok, now that's cleaned up. let's remove duplicate indices (from the TE events)
# preferntially keeping the one that has the "s" event label

# we can use matching_s_events_duplicates to get the index and filename of the "te" events
# that have a matching "s" event
matching_s_events_duplicates <- matching_s_events %>%
    filter(index == index_te)
matching_s_events_duplicates <- matching_s_events_duplicates %>%
    select(index, filename)
matching_s_events_duplicates$event_label <- NA
matching_s_events_duplicates
dat <- dat %>%
    anti_join(matching_s_events_duplicates, by = c("index", "filename", "event_label"))

# now the duplicated data is removed, it's time to address the invalidated trials
# one thing to note is that there were no recorded experiment END events

# let's plot the event data again for each file, we can group all the condition labels
# because they only occur when a specific condition starts, we can call them all
# "condition start"

dat %>%
    mutate(event_label = ifelse(event_label == "t", "condition start", event_label)) %>%
    mutate(event_label = ifelse(event_label == "y", "condition start", event_label)) %>%
    mutate(event_label = ifelse(event_label == "n", "condition start", event_label)) %>%
    filter(!is.na(event_label)) %>%
    ggplot(aes(x = elapsed_time_event, y = as.factor(event_label), color = event_label)) +
    geom_point() +
    facet_wrap(~filename, ncol = 2)

# ok, this looks mostly good, but for some reason there are multiple condition start labels
# for the same condition, let's see if we can find the problem

dat %>%
    filter(event_label %in% c("t", "y", "n")) %>%
    group_by(filename, event_label) %>%
    summarise(count = n())

# it only seems to be a problem with th "n" events, which is ok, we can keep the
# first occurrence of the "n" event that occurs after an "S" event

# first lets find all of the "S" events
S_events <- dat %>%
    filter(event_label == "S")

# now lets find all of the "n" events
n_events <- dat %>%
    filter(event_label == "n")

n_events

# now we can find the first "n" event that occurs after an "S" event

n_events <- n_events %>%
    inner_join(S_events, by = c("filename"), suffix = c("", "_S")) %>%
    filter(index > index_S) %>%
    select(index, filename, event_label) %>%
    group_by(filename) %>%
    slice(1) %>%
    ungroup()

n_events

# now we can set all the other "n" events to NA
dat <- dat %>%
    mutate(event_label = ifelse(event_label == "n" & !index %in% n_events$index, NA, event_label))

# now let's check the event counts again
event_counts_cleaned <- dat %>%
    group_by(event_label) %>%
    summarise(count = n())
event_counts_cleaned

# now let's plot again
dat %>%
    mutate(event_label = ifelse(event_label == "t", "condition start", event_label)) %>%
    mutate(event_label = ifelse(event_label == "y", "condition start", event_label)) %>%
    mutate(event_label = ifelse(event_label == "n", "condition start", event_label)) %>%
    filter(!is.na(event_label)) %>%
    ggplot(aes(x = elapsed_time_event, y = as.factor(event_label), color = event_label)) +
    geom_point() +
    facet_wrap(~filename, ncol = 2)

# althought the counts looked weird, this is correct, because multiple trials
# required that the QTM / Bela, so when sonification didn't work correctly
# we needed to restart (but reorder the conditions to be correct)
# and then there will be an additional S event and a condition start event before and 
# after the restart

# we are getting there, now we only just need to remove any condition
# that has less than 4 s and e events before the next condition start event

# let's get all the events in order for each file
event_timeline <- dat %>%
    filter(!is.na(event_label)) %>%
    group_by(filename) %>%
    ungroup() %>%
    select(event_label, elapsed_time_event, filename, index) %>%
    arrange(filename, index)

event_timeline
# a complete condition should have a condition_start event,
# 4 s events and 4 e events, and no S events within the condition
# so we can use this to find the invalid conditions

# we can match by the order of event_label
event_label_order <- paste(event_timeline$event_label, collapse = "")

valid_trial_regex <- "[n,t,y](se){4}"

valid_trial_ranges = gregexpr(valid_trial_regex, event_label_order)


match_length <- attr(valid_trial_ranges[[1]], "match.length")
match_length <- match_length[1] - 1 # they are all the same

valid_start_indices <- as.integer(valid_trial_ranges[[1]])
valid_end_indices <- valid_start_indices + match_length



# now we slice the event_timeline starting at the valid_start_indices
# and ending at the valid_start_indices + match_length

valid_trials <- data.frame()

for (i in 1:length(valid_start_indices)) {
    # the condition labels are the first character where the match is
    condition_label <- substr(event_label_order, valid_start_indices[i], valid_start_indices[i])
    matched_events <- slice(event_timeline, valid_start_indices[i]:valid_end_indices[i])
    matched_events$condition <- condition_label
    print(paste(matched_events$event_label, matched_events$condition))
    valid_trials <- rbind(matched_events, valid_trials)
}



# order by filename and index
valid_trials <- valid_trials %>%
    arrange(filename, index)

valid_trials

# looks good, we have 99 events, which is indicates one "totally invalid" trial
# (8 start, 8 end events per condition, and 1 condition start event)
# we still have some where the participants stopped moving but we can see that in analysis later

# finally, we need to give each trial within a condition a sequential number
# so we can group them together

valid_trials <- valid_trials %>% group_by(filename, condition, event_label) %>% mutate(trial_number = 1:n()) %>% ungroup()


valid_trials

# cool, now we need to fill it back
# into the original data frame

trial_ranges <- valid_trials %>%
    filter(event_label == "s" | event_label == "e") %>%
    group_by(filename, condition, trial_number) %>%
    summarise(start_index = min(index), end_index = max(index))

# now we have every required to append the tiral number and condition to the original data frame
# all indices between start_index and end_index that match the filename will
# be given the trial number and condition

dat$condition <- NA
dat$trial <- NA
dat$trial_elapsed_time <- NA


for (i in 1:nrow(trial_ranges)) {
    start_index <- trial_ranges$start_index[i]
    end_index <- trial_ranges$end_index[i]
    filename <- trial_ranges$filename[i]
    condition_label <- trial_ranges$condition[i]
    trial_number <- trial_ranges$trial_number[i]
    # get starting elapsed time
    elapsed_start <- dat %>%
        filter(filename == filename & index == start_index) %>%
        select(elapsed_time) %>%
        pull()
    elapsed_start <- elapsed_start[1]
    dat <- dat %>%
        mutate(
            trial = ifelse(
                index >= start_index &
                index <= end_index &
                filename == filename,
                trial_number,
                trial)) %>%
        mutate(
            condition = ifelse(
                index >= start_index &
                index <= end_index &
                filename == filename,
                condition_label,
                condition)) %>%
        mutate(
            trial_elapsed_time = ifelse(
                index >= start_index &
                index <= end_index &
                filename == filename,
                elapsed_time - elapsed_start,
                trial_elapsed_time))
}

# now delete all rows that don't have a condition
dat <- dat %>%
    filter(!is.na(condition))

# now we can plot the y coordinates for each trial by subject
# facet on condition and color by subject
dat$trial <- as.factor(dat$trial)
dat$condition <- as.factor(dat$condition)
dat$subj_w <- as.factor(dat$subj_w)
dat$subj_d <- as.factor(dat$subj_d)

# remove unnecessary columns
dat <- dat %>%
    select(-c(elapsed_time_event, event_label))

dat %>%
    filter(subj_w == "h" & condition == "y") %>%
    ggplot(aes(x = trial_elapsed_time, y = CAR_W_y)) +
    geom_line(color = "blue") +
    geom_line(aes(y = CAR_D_y), color = "red") +
    facet_wrap(~trial, ncol = 2)

dat %>%
    filter(subj_w == "h" & condition == "t") %>%
    ggplot(aes(x = trial_elapsed_time, y = CAR_W_y)) +
    geom_line(color = "blue") +
    geom_line(aes(y = CAR_D_y), color = "red") +
    facet_wrap(~trial, ncol = 2)


dat %>%
    filter(subj_w == "h" & condition == "n") %>%
    ggplot(aes(x = trial_elapsed_time, y = CAR_W_y)) +
    geom_line(color = "blue") +
    geom_line(aes(y = CAR_D_y), color = "red") +
    facet_wrap(~trial, ncol = 2)

# if there's huge chunks of missing data, it probably means the model didn't apply correctly in QTM
# and that a re-export is needed

# let's save this data frame

con <- bzfile("data/combined_data_labeled.tsv.bz2")
write_tsv(dat, con)
