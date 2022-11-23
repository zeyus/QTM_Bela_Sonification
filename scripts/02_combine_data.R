# This file loads all of the cleaned subject data into a single data frame.

library(tidyverse)
library(stringi)

# Load the data in data/Pair{n}/data_*.tsv
# n is the number of the subject pair

# first get a list of all the files

data_files <- Sys.glob("data/data_*.tsv.bz2")

# now we need to load each tsv data file and join events
# then we add filename and combine into a single data frame

print("Loading data")
print(paste("Found", length(data_files), "files"))
print("=====================")

# read in the tsv data
dat <- data.frame()

for (data_file in data_files) {
    print(paste("Processing", data_file))
    # read in the data
    dat_temp <- read_tsv(data_file)
    # get the events
    events_temp <- read_tsv(sub("data_", "events_", data_file))
    # add the events to the data
    dat_temp <- dat_temp %>%
        left_join(events_temp, by = "index", suffix = c("", "_event"))
    # add the filename
    dat_temp <- dat_temp %>%
        mutate(filename = data_file)
    # add to the data frame
    dat <- rbind(dat, dat_temp)
}

head(dat)

# now write the data to a single file
con <- bzfile("data/combined_data.tsv.bz2")
write_tsv(dat, con)
close(con)