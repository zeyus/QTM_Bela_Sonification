# this is where we will align the track's Y coordinates perpendicular to
# the ref markers, we can use the ref markers and the car X,Y,Z coordinates

library(tidyverse)
# load the data
dat <- read_tsv("data/combined_data_labeled.tsv.bz2")

# we need to define some relative positions

base_width <- 1000
base_length <- 1000
base_marker_height <- 510

ref_marker_door_x <- 0
ref_marker_window_x <- 1000

track_door_center_x <- 250
track_window_center_x <- 750

track_width <- 20
track_length <- 1000

track_start_y <- 980
track_end_y <- -180

##### TODO #### 
# VERIFY DIMENSIONS, IF STILL NO TRACK ACCESS, USE QTM
