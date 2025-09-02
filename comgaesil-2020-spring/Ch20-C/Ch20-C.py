from tkinter import *
from tkinter.messagebox import showinfo
from random import shuffle
from PIL import Image, ImageTk

tk = Tk()
main_table = Frame(tk)
conveyor = Frame(tk)

main_table.pack(side=TOP, pady=15)
conveyor.pack(side=BOTTOM)

arrow_canvas = Canvas(tk, width=750, heigh=30, bg='white')
arrow_canvas.pack(side=BOTTOM)
arrow = arrow_canvas.create_polygon((528,13), (528+17,13+17), (528+34,13), outline="black", fill="yellow")
arrow_index = 9
arrow_canvas.create_text((714,23), text="FINAL", fill="red", font="Helvetica 18 bold")

# images of alphabet and picture.
alp_imgs = [Image.open("./alphabet/%d.gif" %i) for i in range(1, 17)]
pic_imgs = [Image.open("./picture/%d.jpg" %i) for i in range(1, 17)]
# randomly match the alphabets to the pictures.
shuffle(pic_imgs)
# resize the images.
alp_imgs = [ImageTk.PhotoImage(img.resize((100,100))) for img in alp_imgs]
conv_imgs = [ImageTk.PhotoImage(img.resize((50,50))) for img in pic_imgs]
pic_imgs = [ImageTk.PhotoImage(img.resize((100,100))) for img in pic_imgs]
# randomly match conveyor's labels to the pictures.
match = list(range(16))
shuffle(match)

def callback(i):
    # reveal a hidden image for 3.0 seconds.
    temp_label = Label(main_table, image=pic_imgs[i], relief="ridge")
    temp_label.grid(row=i//4, column=i%4)
    temp_label.bind("<Button-1>", lambda event, i=i: callback(i))
    tk.after(3000, temp_label.destroy)
    
    global arrow_index

    if conv_labels[arrow_index][1] == i:
        # move the arrow to the right.
        arrow_index += 1
        arrow_canvas.move(arrow, 56, 0)

        # win.
        if arrow_index == 13:   
            showinfo("Game over", "You win.")
            tk.quit()

    elif conv_labels[arrow_index][1] != i:
        # move the arrow and conveyor labels to the left.
        arrow_index -= 1
        arrow_canvas.move(arrow, -56, 0)
        conv_labels.append((Label(conveyor, image=conv_imgs[conv_labels[0][1]], relief="ridge"), conv_labels[0][1]))
        conv_labels[0][0].destroy()
        del conv_labels[0]
        conv_labels[12][0].pack(side=LEFT)
        # prevent circulation.
        temp_list = conv_labels[13:]
        shuffle(temp_list)
        conv_labels[13:] = temp_list

        # lose.
        if arrow_index == -1:
            showinfo("Game over", "You lose.")
            tk.quit()

# alphabet labels.
alp_labels = [None] * 16
for i, img in enumerate(alp_imgs):
    alp_labels[i] = Label(main_table, image=img, relief="ridge")
    alp_labels[i].grid(row=i//4, column=i%4)
    alp_labels[i].bind("<Button-1>", lambda event, i=i: callback(i))

# conveyor labels.
# each label has an index of their picture.
conv_labels = [None] * 16
for i in range(16):
    index = match[i]
    conv_labels[i] = Label(conveyor, image=conv_imgs[index], relief="ridge"), index
    if i <= 12: conv_labels[i][0].pack(side=LEFT)

tk.mainloop()
