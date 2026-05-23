import cv2
import numpy as np
import socket

# PURPOSE: Set up the network connection to send data to the C++ program.
# UDP_IP is the address of this computer.
# UDP_PORT is the door we use to send the messages.
# sock is the actual network tool that will throw the data over the network.
UDP_IP = "127.0.0.1"
UDP_PORT = 5005
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# PURPOSE: Turn on the computer camera.
cap = cv2.VideoCapture(0)

# PURPOSE: Define the color ranges we are looking for.
# This uses the HSV (Hue, Saturation, Value) model.
# Each color has a lower limit and an upper limit to ignore bad lighting.
colors = {
    "Red": [np.array([0, 150, 150]), np.array([10, 255, 255])],
    "Green": [np.array([40, 150, 150]), np.array([80, 255, 255])],
    "Blue": [np.array([100, 150, 150]), np.array([130, 255, 255])]
}

# PURPOSE: The main program loop that runs continuously.
# What it does: It takes pictures from the camera one by one, processes them to find colors,
# and sends the location data to the C++ engine.
while True:
    ret, frame = cap.read()
    if not ret:
        break
    
    # Flip the image so it acts like a mirror
    frame = cv2.flip(frame, 1)
    
    # Apply a blur filter to remove small noise from the camera
    blurred = cv2.GaussianBlur(frame, (11, 11), 0)
    
    # Convert the picture from standard BGR colors to the HSV color model
    hsv = cv2.cvtColor(blurred, cv2.COLOR_BGR2HSV)
    
    detected_color = "None"
    best_area = 0
    best_cx, best_cy = 0, 0
    
    # PURPOSE: Check the camera picture for Red, Green, and Blue.
    # What it does: It looks at the colors dictionary. If it finds a matching color,
    # it draws a shape around it and calculates how big it is and where its center is.
    for color_name, (lower_bound, upper_bound) in colors.items():
        # Create a black and white mask where the target color is white
        mask = cv2.inRange(hsv, lower_bound, upper_bound)
        
        # Find the outlines of the white shapes in the mask
        contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        
        if contours:
            # Pick the biggest shape to ignore small background colors
            c = max(contours, key=cv2.contourArea)
            area = cv2.contourArea(c)
            
            # If the shape is big enough, calculate its center X and Y coordinates
            if area > 1000 and area > best_area:
                best_area = area
                detected_color = color_name
                
                # Math tool to find the exact middle point of the shape
                M = cv2.moments(c)
                if M["m00"] != 0:
                    best_cx = int(M["m10"] / M["m00"])
                    best_cy = int(M["m01"] / M["m00"])
                    
                    # Draw a green box and a red dot on the screen for the user to see
                    x, y, w, h = cv2.boundingRect(c)
                    cv2.rectangle(frame, (x, y), (x+w, y+h), (0, 255, 0), 2)
                    cv2.circle(frame, (best_cx, best_cy), 5, (0, 0, 255), -1)
                    
    # PURPOSE: Send the found data over the network.
    # What it does: If a color was found, it packs the color name, X, Y, and Area into a text message.
    # It sends this text message to the C++ program using the socket.
    if detected_color != "None":
        message = f"{detected_color},{best_cx},{best_cy},{int(best_area)}"
        sock.sendto(message.encode(), (UDP_IP, UDP_PORT))
        
        text = f"Color: {detected_color} | X: {best_cx} | Y: {best_cy} | Area: {int(best_area)}"
        cv2.putText(frame, text, (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255, 255, 255), 2)
    else:
        cv2.putText(frame, "Waiting for color...", (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (200, 200, 200), 2)
        
    cv2.imshow("Camera and Tracking", frame)
    
    # Close the program if the 'q' key is pressed
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()