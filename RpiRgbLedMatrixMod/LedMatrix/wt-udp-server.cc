// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// Small example how to use the library.
// For more examples, look at demo-main.cc
//
// This code is public domain
// (but note, that the led-matrix library this depends on is GPL v2)

#include "led-matrix.h"

#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <string.h> //memset
#include <stdlib.h> //exit(0);
#include <arpa/inet.h>
#include <sys/socket.h>

using rgb_matrix::GPIO;
using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;

#define BUFLEN 512  //Max length of buffer
#define PORT 8888   //The port on which to listen for incoming data

static void DrawOnCanvas(Canvas *canvas) {
  /*
   * Let's create a simple animation. We use the canvas to draw
   * pixels. We wait between each step to have a slower animation.
   */
  canvas->Fill(0, 0, 255);

  int center_x = canvas->width() / 2;
  int center_y = canvas->height() / 2;
  float radius_max = canvas->width() / 2;
  float angle_step = 1.0 / 360;
  for (float a = 0, r = 0; r < radius_max; a += angle_step, r += angle_step) {
    float dot_x = cos(a * 2 * M_PI) * r;
    float dot_y = sin(a * 2 * M_PI) * r;
    canvas->SetPixel(center_x + dot_x, center_y + dot_y,
                     255, 0, 0);
    usleep(1 * 1000);  // wait a little to slow down things.
  }
}

static void DrawPixel(Canvas *canvas, int pixel_nbr) {
	canvas->SetPixel(pixel_nbr / 32, pixel_nbr % 32,
		255, 0, 0);
}

static void Clear(Canvas *canvas) {
	canvas->Fill(0, 0, 0);
}


void die(char *s)
{
	perror(s);
	exit(1);
}

int main(int argc, char *argv[]) {
	/*
	* Set up GPIO pins. This fails when not running as root.
	*/
	GPIO io;
	if (!io.Init())
		return 1;

	/*
	* Set up the RGBMatrix. It implements a 'Canvas' interface.
	*/
	int rows = 32;   // A 32x32 display. Use 16 when this is a 16x32 display.
	int chain = 1;   // Number of boards chained together.
	Canvas *canvas = new RGBMatrix(&io, rows, chain);

	/* 
	* Setup UDP socket 
	*/
	struct sockaddr_in si_me, si_other;

	int s, i, slen = sizeof(si_other), recv_len;
	char buf[BUFLEN];

	//create a UDP socket
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		die("socket");
	}

	// zero out the structure
	memset((char *)&si_me, 0, sizeof(si_me));

	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(PORT);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);

	//bind socket to port
	if (bind(s, (struct sockaddr*)&si_me, sizeof(si_me)) == -1)
	{
		die("bind");
	}

	//keep listening for data
	while (1)
	{
		printf("Waiting for data...");
		fflush(stdout);

		//try to receive some data, this is a blocking call
		memset(&buf, 0, BUFLEN);
		if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == -1)
		{
			die("recvfrom()");
		}

		//print details of the client/peer and the data received
		printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
		printf("Data: %s\n", buf);

		// Set pixel no
		int pixel_nbr = atoi(buf);
		if (pixel_nbr < 0) {
			Clear(canvas);
		}
		else {
			DrawPixel(canvas, pixel_nbr);
		}
		

		//now reply the client with the same data
		if (sendto(s, buf, recv_len, 0, (struct sockaddr*) &si_other, slen) == -1)
		{
			die("sendto()");
		}
	}

	close(s);

	// Shut down the RGB matrix.
	canvas->Clear();
	delete canvas;

	return 0;
}

