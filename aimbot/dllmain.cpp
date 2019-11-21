/*
	The aimbot dll. Responsible for creating a thread within the bzflag application
	which then activates the hack.

	bzflag stores the current player's view angle in the azimuth member of the Player
	class. The angle is stored in its radian representation. Each player also has
	a position vector, which represents their current location on the map. Since 
	this aimbot only aims at players on the ground, pos[2] is ignored, as it represents
	the player's height.

	For each enemy, the position is first normalized by subtracting the enemy's vector from the 
	player's vector. This offsets the player normalized vector to the origin. A representation is
	shown below:
	z
	|
	|
	|	Enemy
	|  /|
	| / |
	|/Θ |
	Player----------- x

	Using geometry, Θ can then be calculated by using Θ = tan^-1( enemy[0] - player[0] / enemy[1] - player[1] ).
	Since inverse tan is an unsigned operation (i.e., tan^-1(45/45) = tan^-1(-45/45), we add PI in cases where 
	the enemy is behind in order to calculate the angle correctly.

	Euclidean distance is used to calculate the closest enemy.

	Since movement in bzflag is reliant on turning the tank, left and right keys were introduced to strafe the 
	player around the current locked-on enemy.
*/
#include <Windows.h>
#include <math.h>

#define M_PI 3.14159265358979323846
#define EUCLID_DIST(X,Y) sqrtf( ( X * X ) + ( Y * Y ) )

// Trimmed down from the bzflag source
struct Player {
	char unknown[0x1ec];
	//170 flag, 4 bytes
	short status;
	float pos[3];
	float velocity[3];
	float azimuth;
};

// Trimmed down from the bzflag source
struct World {
	char unknown[0x10];
	int curMaxPlayers;
	char unknown2[0xc4];
	Player **playerList;
};

Player *player = NULL;
World* world = NULL;

// Since bzflag loads the player and world class dynamically, the offsets are calculated
// based off a static pointer found in CE.
void loadOffsets() {
	HMODULE baseOffset = NULL;
	DWORD *playerOffset = 0;
	DWORD *worldOffset = 0;

	baseOffset = GetModuleHandle("bzflag.exe");
	
	playerOffset = (DWORD*)((DWORD)baseOffset + 0x00253F44);
	player = (Player*)(*playerOffset);

	worldOffset = (DWORD*)((DWORD)baseOffset + 0x0025423C);
	world = (World*)(*worldOffset);
}

DWORD WINAPI injected_thread(LPVOID lpvParam) {
	while (true) {
		// Loop until we have valid player and world classes
		// This only occurs when we join a game
		if (player == NULL || world == NULL) {
			Sleep(100);
			loadOffsets();
			continue;
		}

		// Our strafe functionality
		if (GetAsyncKeyState(VK_RIGHT)) {
			player->pos[0] += 0.1f;
			player->pos[1] += 0.1f;
		}
		else if (GetAsyncKeyState(VK_LEFT)) {
			player->pos[0] -= 0.1f;
			player->pos[1] -= 0.1f;
		}

		// Actual aimbot code
		float closest_player = -1.0f;

		for (int i = 0; i < world->curMaxPlayers; i++) {
			Player* temp = world->playerList[i];
			if (temp == NULL || temp->status != 1)
				continue;

			float abspos0 = temp->pos[0] - player->pos[0];
			float abspos1 = temp->pos[1] - player->pos[1];

			float temp_distance = (float)EUCLID_DIST(abspos0, abspos1);
			if (closest_player == -1 || temp_distance < closest_player) {
				closest_player = temp_distance;
				player->azimuth = atanf(abspos1 / abspos0);
				if (abspos0 < 0)
					player->azimuth += (float)M_PI;
			}
		}
		Sleep(1);
	}

	return 0;
}

bool APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
			CreateThread(NULL, 0, injected_thread, NULL, NULL, NULL);
			break;
	}

	return true;
}
