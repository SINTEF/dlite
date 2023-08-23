TEM data Example
================

Content
-------
- [Background](#background)
- [The case](#the-case)
- [Datasets](#datasets)
- [Setup](#setup)
- [Running the example](#running-the-example)


Background
----------
This example is about studying an Al-Mg-Si aluminium alloy using
transition electron microscopy (TEM).  The aim of such studies is to
understand how the alloy composition and heat treatment influences the
microstructure and how the microstructure in turn influences the alloy
properties.  Based such understanding, new alloys with improved or
tailored properties can be developed.

In this very simple example, we will look at a single Al-Mg-Si alloy
at two conditions, after exposing it for two different heat treatments.

The first heat treatment we denote DA (for direct aged), where the
alloy was solution heat treated for 5 min at 530C and then crunched to
room temperature (RT) and then directly aged for 5 hours at 185C.

![profile-DA](figs/profile-DA.svg)

*Figure: Heat treatment profile, direct aging.*

Two bright-field TEM images were acquired from a sample prepared after
aging (marked with A in the profile above).  The first is a
low-magnification images showing a dense distribution of needle-shaped
beta" precipitates.

![Overview, direct aging](figs/BF_100-at-m5-and-2_001.png)

*Figure: TEM bright-field image; [BF_100-at-m5-and-2_001]*

The second image zooms in to a smaller region.  The dark dots are
beta" precipitates viewed along the needle-direction.

![zoom, direct aging](figs/040.png)

*Figure: TEM bright-field image; [040]*

The second heat treatment we denote NA (for natural aged), where the
alloy after solution heat treatment was kept at RT for 1 month before
aging.  It was also pre-baked for 24h at 90C.

![profile-NA](figs/profile-NA.svg)

*Figure: Heat treatment profile, natural aging.*

An atomic-resolution high angle annular dark-field (HAADF) TEM image
was acquired from a prepared after aging (marked with B in the profile
above).  It shows three beta" precipitates viewed along the needle
direction.

![pre-baked](figs/6c8cm_008.png)

*Figure: High angle annular dark field TEM image; [6c8cm_008]*

By analysing a set of such TEM images, one can characterise the alloy
microstructure in terms of precipitates types and size distributions.

To relate the microstructure to mechanical properties of the alloy,
Vickers hardness measurements was performed at each of the two
conditions.


The case
--------



Datasets
--------
- Raw images.  These images are in Gatan DM3 format and are very large.  They can be downloaded from
  https://folk.ntnu.no/friisj/temdata/.  They can be parsed using the dm3 plugin.
- Alloy composition
- Precipitate statistics
- Hardness curve


Setup
-----
Create a new virtual environment and install needed packages

    pip install -r requirements.txt

Make sure that you have docker and docker-compose installed and the docker deamon is running.
Then start the OTEAPI-services with docker-compose:

    docker-compose pull   # Pull the latest images
    docker-compose up -d  # Run the OTE Services (detached)


Running the example
-------------------
Now you are

   python pipeline.py


[BF_100-at-m5-and-2_001]: https://folk.ntnu.no/friisj/temdata/BF_100-at-m5-and-2_001.dm3
[040]: https://folk.ntnu.no/friisj/temdata/040.dm3
[6c8cm_008]: https://folk.ntnu.no/friisj/temdata/6c8cm_008.dm3
[oteapi-services]: https://github.com/EMMC-ASBL/oteapi-services
