#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Wed Mar  6 12:25:33 2019

@author: mint
"""

import smtplib

# define constant variables
Email = "handsfreemixer445@gmail.com"
Password = "Team@#40"
Server = "imap.gmail.com"


# Function to read and delete the latest email
def send_email(): 
    msg = "\r\n".join(['From: handsfreemixer445@gmail.com', 
                      'To: handsfreemixer445@gmail.com', 
                      'Subject: ERIC', "", 'COKE'])
    server = smtplib.SMTP_SSL('smtp.gmail.com', 465)
    server.ehlo()
    server.login(Email, Password)
    server.sendmail(Email, Email, msg)
    server.quit()
    
for i in range(10):
    send_email()

    
    