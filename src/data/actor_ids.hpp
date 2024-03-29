/* Copyright (C) 2016, Nikolai Wuttke. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

namespace rigel::data
{

enum class ActorID
{
  Hoverbot = 0,
  Explosion_FX_1 = 1,
  Explosion_FX_2 = 2,
  Shot_impact_FX = 3,
  Spiked_green_creature_eye_FX_LEFT = 4,
  Duke_LEFT = 5,
  Duke_RIGHT = 6,
  Duke_rocket_up = 7,
  Duke_rocket_down = 8,
  Duke_rocket_left = 9,
  Duke_rocket_right = 10,
  Smoke_puff_FX = 11,
  Hoverbot_debris_1 = 12,
  Hoverbot_debris_2 = 13,
  Nuclear_waste_can_empty = 14,
  Nuclear_waste_can_debris_1 = 15,
  Nuclear_waste_can_debris_2 = 16,
  Nuclear_waste_can_debris_3 = 17,
  Nuclear_waste_can_debris_4 = 18,
  Green_box_rocket_launcher = 19,
  Green_box_flame_thrower = 20,
  Duke_flame_shot_up = 21,
  Green_box_normal_weapon = 22,
  Green_box_laser = 23,
  Duke_laser_shot_horizontal = 24,
  Duke_laser_shot_vertical = 25,
  Duke_regular_shot_horizontal = 26,
  Duke_regular_shot_vertical = 27,
  Blue_box_health_molecule = 28,
  Menu_font_grayscale = 29,
  Big_green_cat_LEFT = 31,
  Big_green_cat_RIGHT = 32,
  Muzzle_flash_up = 33,
  Muzzle_flash_down = 34,
  Muzzle_flash_left = 35,
  Muzzle_flash_right = 36,
  White_box_circuit_card = 37,
  Wall_mounted_flamethrower_RIGHT = 38,
  Wall_mounted_flamethrower_LEFT = 39,
  Flame_thrower_fire_RIGHT = 40,
  Flame_thrower_fire_LEFT = 41,
  Red_box_bomb = 42,
  Nuclear_explosion = 43,
  Bonus_globe_shell = 44,
  Blue_bonus_globe_1 = 45,
  Blue_bonus_globe_2 = 46,
  Blue_bonus_globe_3 = 47,
  Blue_bonus_globe_4 = 48,
  Watchbot = 49,
  Teleporter_1 = 50,
  Teleporter_2 = 51,
  Rapid_fire_icon = 52,
  White_box_rapid_fire = 53,
  Rocket_launcher_turret = 54,
  Enemy_rocket_left = 55,
  Enemy_rocket_up = 56,
  Enemy_rocket_right = 57,
  Watchbot_container_carrier = 58,
  Watchbot_container = 59,
  Watchbot_container_debris_1 = 60,
  Watchbot_container_debris_2 = 61,
  Bomb_dropping_spaceship = 62,
  Napalm_bomb = 63,
  Bouncing_spike_ball = 64,
  Fire_bomb_fire = 65,
  Electric_reactor = 66,
  Green_slime_blob = 67,
  Green_slime_container = 68,
  Hoverbot_teleport_FX = 69,
  Green_slime_blob_flying_on_ceiling = 70,
  Duke_death_particles = 71,
  Bonus_globe_debris_1 = 72,
  Bonus_globe_debris_2 = 73,
  White_circle_flash_FX = 74,
  Nuclear_waste_can_green_slime_inside = 75,
  Napalm_bomb_small = 76,
  HUD_frame_background = 77,
  Snake = 78,
  Camera_on_ceiling = 79,
  Camera_on_floor = 80,
  Green_hanging_suction_plant = 81,
  META_Appear_only_in_med_hard_difficulty = 82,
  META_Appear_only_in_hard_difficulty = 83,
  Smoke_cloud_FX = 84,
  Reactor_fire_LEFT = 85,
  Reactor_fire_RIGHT = 86,
  Dukes_ship_RIGHT = 87,
  Dukes_ship_LEFT = 88,
  Dukes_ship_after_exiting_RIGHT = 89,
  Dukes_ship_after_exiting_LEFT = 90,
  Dukes_ship_laser_shot = 91,
  Dukes_ship_exhaust_flames = 92,
  Super_force_field_LEFT = 93,
  Biological_enemy_debris = 94,
  Missile_broken = 95,
  Missile_debris = 96,
  Wall_walker = 97,
  Eyeball_thrower_LEFT = 98,
  Eyeball_thrower_RIGHT = 99,
  Eyeball_projectile = 100,
  BOSS_Episode_2 = 101,
  Dynamic_geometry_1 = 102,
  META_Dynamic_geometry_marker_1 = 103,
  META_Dynamic_geometry_marker_2 = 104,
  Dynamic_geometry_2 = 106,
  Messenger_drone_body = 107,
  Messenger_drone_part_1 = 108,
  Messenger_drone_part_2 = 109,
  Messenger_drone_part_3 = 110,
  Messenger_drone_exhaust_flame_1 = 111,
  Messenger_drone_exhaust_flame_2 = 112,
  Messenger_drone_exhaust_flame_3 = 113,
  White_box_cloaking_device = 114,
  Sentry_robot_generator = 115,
  Dynamic_geometry_3 = 116,
  Slime_pipe = 117,
  Slime_drop = 118,
  Force_field = 119,
  Circuit_card_keyhole = 120,
  White_box_blue_key = 121,
  Blue_key_keyhole = 122,
  Score_number_FX_100 = 123,
  Score_number_FX_500 = 124,
  Score_number_FX_2000 = 125,
  Score_number_FX_5000 = 126,
  Score_number_FX_10000 = 127,
  Sliding_door_vertical = 128,
  Keyhole_mounting_pole = 129,
  Blowing_fan = 130,
  Laser_turret = 131,
  Sliding_door_horizontal = 132,
  Respawn_checkpoint = 133,
  Skeleton = 134,
  Enemy_laser_shot_LEFT = 135,
  Enemy_laser_shot_RIGHT = 136,
  Dynamic_geometry_4 = 137,
  Dynamic_geometry_5 = 138,
  Exit_trigger = 139,
  Laser_turret_mounting_post = 140,
  Dynamic_geometry_6 = 141,
  Dynamic_geometry_7 = 142,
  Dynamic_geometry_8 = 143,
  Missile_intact = 144,
  Enemy_laser_muzzle_flash_1 = 147,
  Enemy_laser_muzzle_flash_2 = 148,
  Missile_exhaust_flame = 149,
  Metal_grabber_claw = 150,
  Hovering_laser_turret = 151,
  Metal_grabber_claw_debris_1 = 152,
  Metal_grabber_claw_debris_2 = 153,
  Spider = 154,
  Blue_box_N = 155,
  Blue_box_U = 156,
  Blue_box_K = 157,
  Blue_box_E = 158,
  Blue_guard_RIGHT = 159,
  Blue_box_video_game_cartridge = 160,
  White_box_empty = 161,
  Green_box_empty = 162,
  Red_box_empty = 163,
  Blue_box_empty = 164,
  Yellow_fireball_FX = 165,
  Green_fireball_FX = 166,
  Blue_fireball_FX = 167,
  Red_box_cola = 168,
  Coke_can_debris_1 = 169,
  Coke_can_debris_2 = 170,
  Blue_guard_LEFT = 171,
  Blue_box_sunglasses = 172,
  Blue_box_phone = 173,
  Red_box_6_pack_cola = 174,
  Duke_3d_teaser_text = 175,
  Ugly_green_bird = 176,
  Blue_box_boom_box = 181,
  Blue_box_disk = 182,
  Blue_box_TV = 183,
  Blue_box_camera = 184,
  Blue_box_PC = 185,
  Blue_box_CD = 186,
  Blue_box_M = 187,
  Rotating_floor_spikes = 188,
  Spiked_green_creature_LEFT = 189,
  Spiked_green_creature_RIGHT = 190,
  Spiked_green_creature_eye_FX_RIGHT = 191,
  Spiked_green_creature_stone_debris_1_LEFT = 192,
  Spiked_green_creature_stone_debris_2_LEFT = 193,
  Spiked_green_creature_stone_debris_3_LEFT = 194,
  Spiked_green_creature_stone_debris_4_LEFT = 195,
  Spiked_green_creature_stone_debris_1_RIGHT = 196,
  Spiked_green_creature_stone_debris_2_RIGHT = 197,
  Spiked_green_creature_stone_debris_3_RIGHT = 198,
  Spiked_green_creature_stone_debris_4_RIGHT = 199,
  BOSS_Episode_1 = 200,
  Red_box_turkey = 201,
  Turkey = 202,
  Red_bird = 203,
  Duke_flame_shot_down = 204,
  Duke_flame_shot_left = 205,
  Duke_flame_shot_right = 206,
  Floating_exit_sign_RIGHT = 208,
  Rocket_elevator = 209,
  Computer_Terminal_Duke_Escaped = 210,
  Cloaking_device_icon = 211,
  Lava_pit = 212,
  Messenger_drone_1 = 213,
  Messenger_drone_2 = 214,
  Messenger_drone_3 = 215,
  Messenger_drone_4 = 216,
  Blue_guard_using_a_terminal = 217,
  Super_force_field_RIGHT = 218,
  Smash_hammer = 219,
  Messenger_drone_5 = 220,
  Water_body = 221,
  Lava_fall_1 = 222,
  Lava_fall_2 = 223,
  Water_fall_1 = 224,
  Water_fall_2 = 225,
  Water_drop = 226,
  Water_drop_spawner = 227,
  Water_fall_splash_left = 228,
  Water_fall_splash_center = 229,
  Water_fall_splash_right = 230,
  Lava_fountain = 231,
  Spider_shaken_off = 232,
  Water_surface_1 = 233,
  Water_surface_2 = 234,
  Green_acid_pit = 235,
  Radar_dish = 236,
  Radar_computer_terminal = 237,
  Special_hint_globe_icon = 238,
  Special_hint_globe = 239,
  Special_hint_machine = 240,
  Windblown_spider_generator = 241,
  Spider_debris_2 = 242,
  Spider_blowing_in_wind = 243,
  Unicycle_bot = 244,
  Flame_jet_1 = 246,
  Flame_jet_2 = 247,
  Flame_jet_3 = 248,
  Flame_jet_4 = 249,
  Airlock_death_trigger_LEFT = 250,
  Airlock_death_trigger_RIGHT = 251,
  Floating_exit_sign_LEFT = 252,
  Aggressive_prisoner = 253,
  Explosion_FX_trigger = 254,
  Prisoner_hand_debris = 255,
  Enemy_rocket_2_up = 256,
  Water_on_floor_1 = 257,
  Water_on_floor_2 = 258,
  Enemy_rocket_2_down = 259,
  Blowing_fan_threads_on_top = 260,
  Passive_prisoner = 261,
  Fire_on_floor_1 = 262,
  Fire_on_floor_2 = 263,
  BOSS_Episode_3 = 265,
  Small_flying_ship_1 = 271,
  Small_flying_ship_2 = 272,
  Small_flying_ship_3 = 273,
  Blue_box_T_shirt = 274,
  Blue_box_videocassette = 275,
  BOSS_Episode_4 = 279,
  BOSS_Episode_4_projectile = 280,
  Letter_collection_indicator_N = 290,
  Letter_collection_indicator_U = 291,
  Letter_collection_indicator_K = 292,
  Letter_collection_indicator_E = 293,
  Letter_collection_indicator_M = 294,
  Floating_arrow = 296,
  News_reporter_talking_mouth_animation = 297,
  Rigelatin_soldier = 299,
  Rigelatin_soldier_projectile = 300
};


constexpr auto TOTAL_NUM_ACTOR_IDS = 301;

} // namespace rigel::data
