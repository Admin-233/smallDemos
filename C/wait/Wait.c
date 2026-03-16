#include <stdio.h>
#include <conio.h>
#include <windows.h>

char key = '\0';
int time = 2;

int main()
{
	printf("\nPress p to pause, any other key to exit.");
	printf("\nTime remaining ");
	for(int i = time; i >= 1; i--)
	{
		printf("%d ", i);
		
		int time1 = GetTickCount();
		while(1)
		{
			if (_kbhit())
			{
				key = _getch();
				if (key != 'p' && key != 'P') {
					return 0;
				}
				else
				{
					printf("\nPaused, press e to exit.");
					while(1)
					{
						if (_kbhit())
						{
							key = _getch();
							if (key == 'e' || key == 'E') 
							{
								return 0;
							}
						}
					}
				}
			}
			if(GetTickCount() - time1 >= 1000)
			{
				break;
			}
		}
	}
}