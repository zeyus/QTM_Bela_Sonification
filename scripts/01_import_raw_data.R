library(tidyverse)
library(stringi)


# Load the data in data/Pair{n}/*.tsv
# n is the number of the subject pair

# first get a list of all the files
# originals start with Subj*
data_files <- Sys.glob("data/raw/Subj*.tsv")

# now we need to anonymize the data
# specifically, removing the timestamp
# (this is in the header data starting with TIME_STAMP)
# we also need to extract the events, and make it into
# a proper TSV, we can save the events in a separate file

print("Cleaning data")
print(paste("Found", length(data_files), "files"))
print("=====================")
# read in the data as raw text
for (data_file in data_files) {
  print(paste("Processing", data_file))
  # read in the data
  dat <- readLines(data_file)

  data_file <- sub("raw/", "", data_file)
  data_file <- sub(".tsv", ".tsv.bz2", data_file)

  # remove the timestamp
  events <- stri_extract_first_regex(dat, "^EVENT.*")
  # remove NAs
  events <- events[!is.na(events)]
  # now get other relevant metadata
  # get frequency
  freq <- stri_extract_first_regex(dat, "^FREQUENCY.*")
  freq <- freq[!is.na(freq)]

  # get marker count
  marker_count <- stri_extract_first_regex(dat, "^NO_OF_MARKERS.*")
  marker_count <- marker_count[!is.na(marker_count)]
  marker_count_value <- as.integer(stri_split_fixed(marker_count, "\t")[[1]][2])

  # get frame count
  frame_count <- stri_extract_first_regex(dat, "^NO_OF_FRAMES.*")
  frame_count <- frame_count[!is.na(frame_count)]
  frame_count_value <- as.integer(stri_split_fixed(frame_count, "\t")[[1]][2])

  # get marker names
  marker_names <- stri_extract_first_regex(dat, "^MARKER_NAMES.*")
  marker_names <- marker_names[!is.na(marker_names)]
  marker_names_values <- stri_split_fixed(
    marker_names,
    "\t")[[1]][1:marker_count_value + 1]

  # now just keep the tracking information
  dat <- stri_extract_first_regex(dat, "^[0-9]+\t.*")
  dat <- dat[!is.na(dat)]

  # now ensure the number of frames is correct
  # this is the number of lines in the data
  assertthat::assert_that(
    length(dat) == frame_count_value,
    msg = "Number of frames is not correct")

  # ensure the number of markers is correct
  # this is 3 columns per marker, plus index and time
  assertthat::assert_that(
    length(stri_split_fixed(dat[[1]], "\t")[[1]]) == marker_count_value * 3 + 2,
    msg = "Number of markers is not correct")

  print(paste("File has", length(dat), "frames"))
  print(paste("File has", marker_count_value, "markers"))

  print("Writing data files")

  # now we need to add a header row to the data
  # this is:
  # index\telapsed_time\t{marker_names}[1]x\t
  # {marker_names}[1]y\t{marker_names}[1]z ...
  # where {marker_names}[1] is the first marker name
  header <- c(
    "index",
    "elapsed_time",
    paste0(
      rep(marker_names_values, each = 3),
      c("_x", "_y", "_z")))
  header <- paste0(header, collapse = "\t")
  dat <- c(header, dat)

  # write the data back out in the same directory but prefix with anon_
  data_out_filename <- gsub("Subj", "data_Subj", data_file)
  print(paste("Writing", data_out_filename))
  con <- bzfile(data_out_filename)
  writeLines(dat, con)
  close(con)

  # add header to events
  events <- c("type\tevent_label\tindex\telapsed_time", events)
  # write the events out to a separate file
  event_out_filename <- gsub("Subj", "events_Subj", data_file)
  print(paste("Writing", event_out_filename))
  con <- bzfile(event_out_filename)
  writeLines(events, con)
  close(con)

  # write the metadata out to a separate file
  metadata <- c(freq, marker_count, frame_count, marker_names)
  metadata_out_filename <- gsub("Subj", "metadata_Subj", data_file)
  print(paste("Writing", metadata_out_filename))
  con <- bzfile(metadata_out_filename)
  writeLines(metadata, con)
  close(con)

}
print("Done")
