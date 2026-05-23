*HOW TO RUN:*

First, make sure you have Python installed. You will need to open your terminal and install opencv-python and numpy.
You also need Visual Studio installed for the C++ part, with the SDL3 library downloaded.

Open the tracker.py file on Visual Studio Code and click run at the top. If doesn't work, open a terminal in the project folder and type "python tracker.py".

Open the C++ project file in Visual Studio and start the program.

Open a fully red, blue, or green image on your phone screen and hold it up to the camera. You will see the 3D shapes react on your computer screen.

*HOW TO CLOSE:*

To close the Python camera, click on the video window to select it, and then press the q key on your keyboard.
To close the C++ graphics window, go back to Visual Studio and click the red Stop Debugging button at the top.

*ABOUT THE PROJECT:*

This project brings together the image processing and 3D graphics. It consists of two parts working together.

The first part is a Python script using OpenCV. It opens the webcam, applies a blur filter, and uses the HSV color space to find a specific color on the screen. This covers the material from our image processing and color model labs.

The second part is a C++ program using SDL3. It uses Bresenham midpoint line drawing algorithm, 3D homogeneous coordinates and matrix multiplication to scale, rotate, and project 3D shapes onto the 2D screen.

The Python program finds the X and Y coordinates of the color and sends them to the C++ program over a local UDP socket.

If you show a red screen to the camera, the C++ program draws a 3D cube. If you show a blue screen, it draws a 3D pyramid. If you show a green screen, a cube spins on its own. Moving the phone closer to the camera makes the area bigger, which scales the 3D shape up. Moving it left or right rotates the shape.