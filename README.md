# mehs
Multi-Event HTTP Server
=======================

Really, really, really barebones HTTP server, doesn't do anything except return a default
page and ignores pretty much everything except the keyword "GET".

At this stage it's just a proof-of-concept that uses libev. Another one of my learning
exercises. Maybe it'll grow into something useful later.

Compile with "make". Run the resulting binary (mehs), and connects on port 8080. You
can change these settings in the source code.
