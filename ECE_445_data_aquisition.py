"""
Created on Tue Mar  5 17:58:41 2019

@author: Eric Mysliwiec

Sample code used as reference: 
    https://codehandbook.org/how-to-read-email-from-gmail-using-python/
    http://www.jonwitts.co.uk/archives/896
    
The following code should receive data from our drink mixer email. When a user
wants to close their tab press Enter and then type in their name to print their
drink purchases and the total cost. 
"""

import numpy, smtplib, time, imaplib, email
import sys, termios, tty, os
import imaplib, email


# define constant variables
Email = "handsfreemixer445@gmail.com"
Password = "Team@#40"
Server = "imap.gmail.com"
button_delay = 0.2

# Function to read and delete the latest email
def read_latest_email():
    # mail reading logic
    mail = imaplib.IMAP4_SSL(Server)
    mail.login(Email, Password)
    mail.select('inbox')
    
    type, data = mail.search(None, 'ALL')
    mail_ids = data[0]
       
    id_list = mail_ids.split()
    if(len(id_list) == 0):
        return 'N/A', 'N/A'
    latest_email_id = id_list[-1]    
    message = []
    
    typ,data = mail.fetch(latest_email_id, '(RFC822)')
        
    for response_part in data:
        if isinstance(response_part, tuple):
            msg = email.message_from_string(response_part[1].decode('utf-8'))
            email_subject = msg['subject']
            
            for x in msg.walk():
                if(x.get_content_type() == 'text/plain'):
                    message.append(x.get_payload())
            
            email_message = ''.join(message)

    mail.store(latest_email_id, '+FLAGS', '\\Deleted')
    
    return email_subject, email_message

# Function to check what has been pressed
def getch():
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    try:
        tty.setstraw(sys.stdin.fileno())
        ch = sys.stdin.read(1)
        
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
    return ch

# Main function to run until interupted
while true:
    char = getch()
    
    if(char == "Enter"):
        name = raw_input("PLEASE ENTER CUSTOMER NAME: ")
        
    email_subject, email_message = read_latest_email()
    
    print(email_subject)
    print(email_message)

