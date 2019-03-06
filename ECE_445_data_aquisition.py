"""
Created on Tue Mar  5 17:58:41 2019

@author: Eric Mysliwiec

Sample code used as reference: 
    https://codehandbook.org/how-to-read-email-from-gmail-using-python/
    http://www.jonwitts.co.uk/archives/896
    https://ubuntuforums.org/showthread.php?t=2394609
    
The following code should receive data from our drink mixer email. When a user
wants to close their tab press Enter and then type in their name to print their
drink purchases and the total cost. 
"""

import numpy, smtplib, time, imaplib, email

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

# Main function to run until interupted     
boolean = 0
tab = {"initialize": ["LEMONADE", "COKE"]}
while (boolean == 0):
    
    try:
        while (boolean == 0):
            email_subject, email_message = read_latest_email()
            if(email_subject != 'N/A'):
                if(email_subject in tab.keys()):
                    tab[email_subject].append(email_message)
                    print(email_subject)
                    print(email_message)   
                else:
                    tab[email_subject] = [email_message]
            time.sleep(button_delay)
    
    except KeyboardInterrupt:
        julian = 0
        while(julian == 0):
            data = input("PLEASE ENTER CUSTOMER NAME: ")
            name = data.upper()
            if(name in tab.keys()):
                print('NAME:')
                print(name)
                print('DRINKS PURCHASED:')
                for i in tab[name]:
                    print(i)
                print('ORDER TOTAL:')
                print(len(tab[name]))
                tab.pop(name, None)
                julian = 1
            if(name == "QUIT"):
                julian = 1
            else:
                print('ERROR: NAME NOT FOUND')

