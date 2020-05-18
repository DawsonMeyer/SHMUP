#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#include <iostream>

using namespace std;

class SHMUP : public olc::PixelGameEngine
{
public:
	SHMUP()
	{
		sAppName = "SHMUP";
	}

	olc::vf2d vPlayerPos;
	olc::Sprite* sprPlayer;
	olc::Sprite* sprEnemy[4];

	float playerSpeed = 200.0f;
	float worldSpeed = 20.0f;
	float playerHealth = 1000.0f;
	float gunReloadTimer = 0.0f;
	float gunReloadDelay = 0.2f;

	double worldPos = 0;

	array<olc::vf2d, 1000> starArr;

	struct Enemy;
	struct Bullet;

	struct EnemyDefinition
	{
		double triggerTime = 0.0f;
		uint32_t spriteID = 0;
		float health = 0.0f;
		float offset = 0.0f; //decides where the enemies spawn along x

		function<void(Enemy&, float, float)> funcMove;
		function<void(Enemy&, float, float, list<Bullet>&)> funcFire;
	};

	struct Enemy
	{
		olc::vf2d pos;
		EnemyDefinition def;

		array<float, 4> moveData{ 0 };
		array<float, 4> fireData{ 0 };

		void Update(float fElapsedTime, float scrollSpeed, list<Bullet>& b)
		{
			def.funcMove(*this, fElapsedTime, scrollSpeed);
			def.funcFire(*this, fElapsedTime, scrollSpeed, b);
		}
	};

	struct Bullet
	{
		olc::vf2d pos;
		olc::vf2d vel;
		bool remove = false;
	};

	olc::vf2d GetMiddle(const olc::Sprite *s)
	{
		return { (float)s->width, (float)s->height };
	}

	bool canFire = false;

	list<EnemyDefinition> spawnList;
	list<Enemy> enemyList;
	list<Enemy> bossList;
	list<Bullet> bulletList;
	list<Bullet> bulletListPlayer;
	
	list<Bullet> fragmentList;

	bool OnUserCreate() override
	{
		sprPlayer = new olc::Sprite("Art//SHMUP_Player.png");
		sprEnemy[0] = new olc::Sprite("Art//Enemy1.png");
		sprEnemy[1] = new olc::Sprite("Art//Enemy2.png");
		sprEnemy[2] = new olc::Sprite("Art//Enemy3.png");
		sprEnemy[3] = new olc::Sprite("Art//Boss.png");

		vPlayerPos = { (float)ScreenWidth() / 2, (float)ScreenHeight() / 2};
		for (auto &s : starArr) s = { (float)(rand() % ScreenWidth()), (float)(rand() % ScreenHeight()) };

		//Movement Patterns
		auto Move_None = [&](Enemy &e, float fElapsedTime, float scrollSpeed)
		{
			e.pos.y += scrollSpeed * fElapsedTime;
		};

		auto Move_Fast = [&](Enemy &e, float fElapsedTime, float scrollSpeed)
		{
			e.pos.y += scrollSpeed * fElapsedTime * 3.0f;
		};

		auto Move_WaveNarrow = [&](Enemy &e, float fElapsedTime, float scrollSpeed)
		{
			e.pos.y += scrollSpeed * fElapsedTime * 1.0f;
			e.moveData[0] += fElapsedTime;
			e.pos.x += 50.0f * cosf(e.moveData[0]) *fElapsedTime;
		};

		auto Move_WaveWide = [&](Enemy &e, float fElapsedTime, float scrollSpeed)
		{
			e.pos.y += scrollSpeed * fElapsedTime * 1.0f;
			e.moveData[0] += fElapsedTime;
			e.pos.x += 200.0f * cosf(e.moveData[0]) *fElapsedTime;
		};

		auto Move_Boss = [&](Enemy &e, float fElapsedTime, float scrollSpeed)
		{
			if (e.pos.y <= 100.0f)
			{
				e.pos.y += scrollSpeed * fElapsedTime * 1.0f;
			}
			else
			{
				e.moveData[0] += fElapsedTime;
				e.pos.x += 100.0f * cosf(e.moveData[0]) * fElapsedTime;
			}
		};


		//Firing Patterns
		auto Fire_None = [&](Enemy &e, float fElapsedTime, float scrollSpeed, list<Bullet> &bullets)
		{
	
		};
		
		auto Fire_Straight = [&](Enemy &e, float fElapsedTime, float scrollSpeed, list<Bullet> &bullets)
		{
			constexpr float delay = 0.5f;
			e.fireData[0] += fElapsedTime;
			if (e.fireData[0] >= delay)
			{
				e.fireData[0] -= delay;
				Bullet b;
				b.pos = e.pos + GetMiddle(sprEnemy[e.def.spriteID]);
				b.vel = { 0.0f, 180.0f };
				bullets.push_back(b);
			}
		};

		auto Fire_CirclePulse = [&](Enemy &e, float fElapsedTime, float scrollSpeed, list<Bullet> &bullets)
		{
			constexpr float delay = 0.5f;
			constexpr int nBullets = 30;
			constexpr float theta = 3.14159f * 2.0f / (nBullets);
			e.fireData[0] += fElapsedTime;
			if (e.fireData[0] >= delay)
			{
				e.fireData[0] -= delay;
				for (int i = 0; i < nBullets; i++)
				{
					Bullet b;
					b.pos = e.pos + GetMiddle(sprEnemy[e.def.spriteID]);
					b.vel = { 180.0f * cos(theta * i), 180.0f * sin(theta * i) };
					bullets.push_back(b);
				}
			}
		};

		auto Fire_DeathSpiral = [&](Enemy &e, float fElapsedTime, float scrollSpeed, list<Bullet> &bullets)
		{
			constexpr float delay = 0.02f;
			constexpr int nBullets = 30;
			constexpr float theta = 3.14159f * 2.0f / (nBullets);
			e.fireData[0] += fElapsedTime;
			if (e.fireData[0] >= delay)
			{
				e.fireData[0] -= delay;
				e.fireData[1] += 1.0f;

				Bullet b;
				b.pos = e.pos + GetMiddle(sprEnemy[e.def.spriteID]);
				b.vel = { 180.0f * cos(e.fireData[1]), 180.0f * sin(e.fireData[1]) };
				bullets.push_back(b);
			}
		};

		spawnList = 
		{
			{60.0, 0, 3.0f, 0.5f, Move_None, Fire_CirclePulse},
			{240.0, 1, 3.0f, 0.25f, Move_WaveNarrow, Fire_CirclePulse},
			{240.0, 1, 3.0f, 0.75f, Move_WaveNarrow, Fire_Straight},
			{360.0, 2, 3.0f, 0.2f, Move_None, Fire_Straight},
			{360.0, 2, 3.0f, 0.5f, Move_Fast, Fire_Straight},
			{360.0, 2, 3.0f, 0.8f, Move_None, Fire_Straight},
			{500.0, 2, 3.0f, 0.5f, Move_Fast, Fire_CirclePulse},
			{750.0, 3, 30.0f, 0.5f, Move_Boss, Fire_DeathSpiral},
		};

		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		vPlayerPos.y += worldSpeed * fElapsedTime;

		//controls
		if (GetKey(olc::W).bHeld && GetKey(olc::A).bHeld)
		{
			sprPlayer = new olc::Sprite("Art//Accel_Player.png");
			vPlayerPos.x -= (playerSpeed + worldSpeed) * fElapsedTime;
			vPlayerPos.y -= (playerSpeed + worldSpeed)* fElapsedTime;
		}
		else if (GetKey(olc::W).bHeld && GetKey(olc::D).bHeld)
		{
			sprPlayer = new olc::Sprite("Art//Accel_Player.png");
			vPlayerPos.x += playerSpeed * fElapsedTime;
			vPlayerPos.y -= playerSpeed * fElapsedTime;
		}
		else if (GetKey(olc::S).bHeld && GetKey(olc::A).bHeld)
		{
			sprPlayer = new olc::Sprite("Art//Stop_Player.png");
			vPlayerPos.x -= playerSpeed * fElapsedTime;
			vPlayerPos.y += playerSpeed * fElapsedTime;
		}
		else if (GetKey(olc::S).bHeld && GetKey(olc::D).bHeld)
		{
			sprPlayer = new olc::Sprite("Art//Stop_Player.png");
			vPlayerPos.x += playerSpeed * fElapsedTime;
			vPlayerPos.y += playerSpeed * fElapsedTime;
		}
		else if (GetKey(olc::W).bHeld)
		{
			sprPlayer = new olc::Sprite("Art//Accel_Player.png");
			vPlayerPos.y -= playerSpeed * fElapsedTime;
		}
		else if (GetKey(olc::A).bHeld)
		{
			sprPlayer = new olc::Sprite("Art//Left_Player.png");
			vPlayerPos.x -= playerSpeed * fElapsedTime;
		}
		else if (GetKey(olc::S).bHeld)
		{
			sprPlayer = new olc::Sprite("Art//Stop_Player.png");
			vPlayerPos.y += playerSpeed * fElapsedTime;
		}
		else if (GetKey(olc::D).bHeld)
		{
			sprPlayer = new olc::Sprite("Art//Right_Player.png");
			vPlayerPos.x += playerSpeed * fElapsedTime;
		}
		else
		{
			sprPlayer = new olc::Sprite("Art//SHMUP_Player.png");
		}

		//make sure the player doesnt go out of bounds
		if (vPlayerPos.x <= 0) vPlayerPos.x = 0;
		if (vPlayerPos.x + 36.0f >= (float)ScreenWidth()) vPlayerPos.x = (float)ScreenWidth() - 36.0f;
		if (vPlayerPos.y <= 0) vPlayerPos.y = 0;
		if (vPlayerPos.y + 36.0f >= (float)ScreenHeight()) vPlayerPos.y = (float)ScreenHeight() - 36.0f;

		gunReloadTimer += fElapsedTime;

		if (gunReloadTimer >= gunReloadDelay)
		{
			canFire = true;
			gunReloadTimer -= gunReloadDelay;
		}

		if (GetKey(olc::SPACE).bHeld || GetMouse(0).bHeld)
		{
			if (canFire)
			{
				Bullet b;
				b.pos = { vPlayerPos.x + 16, vPlayerPos.y };
				b.vel = { 0.0, -300.0f };
				bulletListPlayer.push_back(b);
				canFire = false;
			}
		}

		//--------
		//updates
		//--------
		worldPos += worldSpeed * fElapsedTime;

		//enemy spawns 
		while (!spawnList.empty() && worldPos >= spawnList.front().triggerTime)
		{
			Enemy e;
			e.def = spawnList.front();
			e.pos = {
				e.def.offset * ((float)ScreenWidth()) - 0.0f - (float)sprEnemy[e.def.spriteID]->width / 2,
				0.0f - (float)sprEnemy[e.def.spriteID]->height
			};

			spawnList.pop_front();

			if (e.def.spriteID == 3)//changed for boss
			{
				bossList.push_back(e);
			}
			else {

				enemyList.push_back(e);
			}
		}

		//Update explosion fragments
		for (auto &f : fragmentList) f.pos += (f.vel + olc::vf2d(0.0f, worldSpeed)) * fElapsedTime;

		//Update Enemies
		for (auto& e : enemyList) e.Update(fElapsedTime, worldSpeed, bulletList);
		for (auto& e : bossList) e.Update(fElapsedTime, worldSpeed, bulletList);

		//Update Enemy Bullets
		for (auto& b : bulletList)
		{
			b.pos += (b.vel + olc::vf2d(0.0f, worldSpeed)) * fElapsedTime;

			if ((b.pos - (vPlayerPos + olc::vf2d(16, 16))).mag2() < 16.0f*16.0f)
			{
				b.remove = true;
				playerHealth -= 50.0f;
				//added fragments 
				if (playerHealth <= 0)
				{
					float angle;
					float speed;
					for (int i = 0; i < 500; i++)
					{
						angle = ((float)rand() / (float)RAND_MAX) * 2.0f * 3.14159f;
						speed = ((float)rand() / (float)RAND_MAX) * 200.0f + 50.0f;
						fragmentList.push_back(
							{
								vPlayerPos + olc::vf2d(16, 16),
								{speed * cos(angle), speed * sin(angle)}
							});
					}
				}
			}
		}

		//Update Player Bullets
		for (auto& b : bulletListPlayer)
		{
			b.pos += (b.vel + olc::vf2d(0.0f, worldSpeed)) * fElapsedTime;

			for (auto &e : enemyList)
			{
				if ((b.pos - (e.pos + olc::vf2d(16, 16))).mag2() < 16.0f*16.0f)
				{
					b.remove = true;
					e.def.health -= 1.0f;

					if (e.def.health <= 0)
					{
						float angle;
						float speed;
						for (int i = 0; i < 500; i++)
						{
							angle = ((float)rand() / (float)RAND_MAX) * 2.0f * 3.14159f;
							speed = ((float)rand() / (float)RAND_MAX) * 200.0f + 50.0f;
							fragmentList.push_back(
								{
									e.pos + olc::vf2d(16, 16),
									{speed * cos(angle), speed * sin(angle)}
								});
						}
					}
				}
			}
			for (auto &e : bossList)
			{
				if ((b.pos - (e.pos + olc::vf2d(36, 36))).mag2() < 36.0f*36.0f)
				{
					b.remove = true;
					e.def.health -= 1.0f;

					if (e.def.health <= 0)
					{
						float angle;
						float speed;
						for (int i = 0; i < 500; i++)
						{
							angle = ((float)rand() / (float)RAND_MAX) * 2.0f * 3.14159f;
							speed = ((float)rand() / (float)RAND_MAX) * 200.0f + 50.0f;
							fragmentList.push_back(
								{
									e.pos + olc::vf2d(16, 16),
									{speed * cos(angle), speed * sin(angle)}
								});
						}
					}
				}
			}
		}

		//Remove enemies that go off screen or when health == 0
		enemyList.remove_if([&](const Enemy& e) {return (e.pos.y >= (float)ScreenHeight()) || e.def.health <= 0 || playerHealth <= 0; }); //player dead
		bossList.remove_if([&](const Enemy& e) {return (e.pos.y >= (float)ScreenHeight()) || e.def.health <= 0; });

		//remove bullets
		bulletList.remove_if([&](const Bullet& b) {return (b.pos.x < 0 || b.pos.x > (float)ScreenWidth() || b.pos.y >= (float)ScreenHeight()) || b.pos.y < 0 || b.remove || playerHealth <= 0; });
		bulletListPlayer.remove_if([&](const Bullet& b) {return (b.pos.x < 0 || b.pos.x >(float)ScreenWidth() || b.pos.y >= (float)ScreenHeight()) || b.pos.y < 0 || b.remove || playerHealth <= 0; });

		fragmentList.remove_if([&](const Bullet& b) {return (b.pos.x < 0 || b.pos.x >(float)ScreenWidth() || b.pos.y >= (float)ScreenHeight()) || b.pos.y < 0 || b.remove; });


		//-------
		//display
		//-------
		//last thing drawn is on top
		Clear(olc::BLACK);

		//stars
		for (size_t i = 0; i < starArr.size(); i++)
		{
			auto &s = starArr[i];

			s.y += worldSpeed * fElapsedTime * ((i < 300) ? 0.8 : 1);
			if (s.y > ScreenHeight())
			{
				s = { (float)(rand() % ScreenWidth()), 0.0f };
			}

			Draw(s, ((i < 300) ? olc::DARK_GREY : olc::WHITE));
		}


		//draw sprites
		SetPixelMode(olc::Pixel::MASK);
		if (playerHealth > 0)
		{
			DrawSprite(vPlayerPos, sprPlayer, 2); //third arg is for scale
		}
		else
		{
			DrawString(280, 240, "GAME OVER, PAL");
		}
		
		for (auto &e : enemyList) DrawSprite(e.pos, sprEnemy[e.def.spriteID], 2);
		for (auto &e : bossList) DrawSprite(e.pos, sprEnemy[e.def.spriteID], 2);
		SetPixelMode(olc::Pixel::NORMAL);


		//enemy bullets
		for (auto &b : bulletList) FillCircle(b.pos, 3, olc::RED);

		//player bullets
		for (auto &b : bulletListPlayer) FillCircle(b.pos, 3, olc::DARK_CYAN);

		//explosion fragments
		for (auto &f : fragmentList) Draw(f.pos, olc::DARK_YELLOW);

		DrawString((ScreenHeight() + 16) - ScreenHeight(), (ScreenHeight() + 4) - ScreenHeight(), "HEALTH:");
		FillRect((ScreenHeight() + 76) - ScreenHeight(), (ScreenHeight() + 4) - ScreenHeight(), (playerHealth/2), 8, olc::GREEN);
		
		for (auto &e : bossList)
		{
			DrawString(16, 460, "HEDMAN:");
			FillRect(76, 460, e.def.health * 19, 8, olc::DARK_RED);
		}

		if (spawnList.empty() && bossList.empty())
		{
			DrawString(280, 240, "YOU WIN!!!!!!");
		}
		
		return true;
	}

private:

};

int main()
{
	SHMUP demo;
	if (demo.Construct(640, 480, 2, 2))
		demo.Start();

	return 0;
}
