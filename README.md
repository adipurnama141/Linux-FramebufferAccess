# ComputerGraphics - Linux Framebuffer Access
Program C untuk mengakses framebuffer pada linux.

## Pendahuluan
Framebuffer adalah segmen khusus pada memori yang berisi nilai pixel pada layar. Simpelnya, dengan mengubah-ubah isi framebuffer, kita dapat langsung "mewarnai" layar monitor kita, pixel tiap pixel.

Program ini (graf.c) akan mengakses framebuffer pada linux untuk menggambar pola huruf k pada layar. Pola pixel huruf k disimpan pada file k.txt , file ini berisi nilai intensitas warna pada tiap pixel (255 = putih , 0 = hitam)

## Cara menggunakan
1. Kompilasi file graf.c
2. Masuk ke linux console mode, caranya tekan Ctrl + Alt + F2. Untuk keluar , tekan alt + f7
3. Masukkan command "sudo su". Untuk mengakses framebuffer, dibutuhkan akses superuser.
4. Eksekusi program graf.c
