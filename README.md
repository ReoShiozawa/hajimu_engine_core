# はじむ Engine Core (jp-engine_core)

**はじむ版 Unity** — C++20/23 によるデータ指向ゲームエンジンコア

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-1.0.0-green.svg)]()
[![C++](https://img.shields.io/badge/C%2B%2B-20%2F23-orange.svg)]()

## 概要

`engine_core` は[はじむプログラミング言語](https://github.com/ReoShiozawa/hajimu)のゲームエンジンコアパッケージです。
Archetype ECS、Work-Stealing JobSystem、RenderGraph、VFS、物理エンジン、オーディオ、ネットワークなど、
モダンなゲームエンジンに必要な基盤をすべて C++20/23 で実装しています。

**はじむスクリプトから完全に日本語 API で操作可能です。**

```
取り込む "engine_core" として エンジン

エンジン["エンジン初期化"]()
変数 プレイヤー = エンジン["エンティティ作成"]()
エンジン["シーン追加"](プレイヤー, "プレイヤー")
エンジン["物理ボディ追加"](プレイヤー, 1.0)
```

## アーキテクチャ

```
┌─────────────────────────────────────────────────┐
│              はじむ Script Layer                  │
│   (日本語 API: エンティティ作成, 物理ステップ...)  │
├────────────────────┬────────────────────────────┤
│   Plugin Bridge    │     engine_core.hjp         │
│   (plugin.cpp)     │     (shared library)        │
├────────────────────┴────────────────────────────┤
│                     C++20/23                     │
├──────┬──────┬──────┬──────┬──────┬──────┬──────┤
│ Core │ ECS  │Input │Scene │ Res  │Render│Phys  │
│      │      │      │      │      │      │      │
│types │entity│input │graph │ vfs  │graph │world │
│memory│comp  │action│trans │asset │back  │      │
│log   │arch  │  map │prefab│mgr   │shader│      │
│refl  │world │      │      │      │      │      │
│task  │sys   │      │      │      │      │      │
│      │query │      │      │      │      │      │
│      │cmd   │      │      │      │      │      │
├──────┴──────┴──────┴──────┴──────┴──────┴──────┤
│  Audio  │  Network  │  Script  │  Build         │
│ spatial │ sync/roll │ hajimu   │ pipeline       │
│ mixing  │  back     │ FFI/hot  │ cooking        │
└─────────┴───────────┴──────────┴────────────────┘
```

## モジュール一覧

| モジュール | ヘッダ | 説明 |
|:--|:--|:--|
| **Core** | `core/types.hpp` | 基本型, Vec2/3/4, Mat4, Quat, Color, Result, Concepts |
|  | `core/memory.hpp` | Arena, Frame, Pool, Linear アロケータ |
|  | `core/log.hpp` | レベル付きロガー (色付きコンソール + ファイル) |
|  | `core/reflection.hpp` | 型情報レジストリ (ENG_REFLECT マクロ) |
|  | `core/task_graph.hpp` | Work-Stealing JobSystem + DAG TaskGraph |
| **ECS** | `ecs/entity.hpp` | Entity ハンドル (Index + Generation) |
|  | `ecs/component.hpp` | SoA ComponentColumn |
|  | `ecs/archetype.hpp` | Archetype テーブル (SoA レイアウト) |
|  | `ecs/world.hpp` | World (Entity/Component/Archetype 管理) |
|  | `ecs/system.hpp` | システムスケジューラ (トポロジカルソート) |
|  | `ecs/query.hpp` | 型安全クエリ (`for_each<Pos, Vel>(...)`) |
|  | `ecs/command_buffer.hpp` | 遅延コマンドバッファ (スレッドセーフ) |
| **Input** | `input/input_system.hpp` | キーボード/マウス/ゲームパッド |
|  | `input/action_map.hpp` | Action ベースマッピング (日本語アクション名) |
| **Scene** | `scene/scene_graph.hpp` | 親子階層, 深さ優先走査 |
|  | `scene/transform.hpp` | Transform + WorldTransform |
|  | `scene/prefab.hpp` | プレハブ (Entity テンプレート) |
| **Resource** | `resource/vfs.hpp` | VFS (`res://` パス, ZIP/メモリ対応) |
|  | `resource/asset_handle.hpp` | 参照カウント付きアセットハンドル |
|  | `resource/resource_manager.hpp` | 非同期ローダー + GC |
| **Render** | `render/render_graph.hpp` | RenderGraph (パス依存グラフ) |
|  | `render/render_backend.hpp` | GPU 抽象インターフェース |
|  | `render/shader_compiler.hpp` | SPIRV/MSL/HLSL/WGSL クロスコンパイル |
| **Physics** | `physics/physics_world.hpp` | 2D/3D 物理 (レイキャスト, コリジョン) |
| **Audio** | `audio/audio_system.hpp` | 空間オーディオ + ミキシング |
| **Network** | `network/net_system.hpp` | 状態同期 + ロールバックネットコード |
| **Script** | `script/script_runtime.hpp` | はじむ FFI ブリッジ + ホットリロード |
| **Build** | `build/build_pipeline.hpp` | アセットクッキング + マルチターゲット |

## はじむ API (37関数)

| カテゴリ | 関数名 | 引数 |
|:--|:--|:--|
| エンジン | `エンジン初期化` | なし |
| | `エンジン終了` | なし |
| | `エンジンバージョン` | なし |
| ECS | `エンティティ作成` | なし |
| | `エンティティ削除` | エンティティID |
| | `エンティティ生存確認` | エンティティID |
| | `エンティティ数` | なし |
| | `アーキタイプ数` | なし |
| | `コマンド実行` | なし |
| シーン | `シーン追加` | エンティティID [, 名前 [, 親ID]] |
| | `シーン削除` | エンティティID |
| | `シーン検索` | 名前 |
| | `シーン親変更` | エンティティID, 親ID |
| | `シーンノード数` | なし |
| 入力 | `キー押下中` | キーコード |
| | `キー押下` | キーコード |
| | `マウス座標` | なし |
| | `入力フレーム開始` | なし |
| | `アクション押下` | アクション名 |
| | `アクション押下中` | アクション名 |
| VFS | `VFSマウント` | プレフィックス, 実パス [, 優先度] |
| | `VFS読込` | 仮想パス |
| | `VFS存在確認` | 仮想パス |
| 物理 | `物理ステップ` | [dt] |
| | `物理ボディ追加` | エンティティID [, 質量] |
| | `物理力適用` | エンティティID, fx, fy, fz |
| | `物理同期` | なし |
| レンダー | `レンダーパス追加` | パス名 |
| | `レンダーコンパイル` | なし |
| | `レンダー実行` | なし |
| | `レンダークリア` | なし |
| ログ | `ログ情報` | メッセージ |
| | `ログ警告` | メッセージ |
| | `ログエラー` | メッセージ |

## ビルド

### 必要要件

- C++20 対応コンパイラ (Clang 16+, GCC 13+, MSVC 2022+)
- CMake 3.25+
- はじむ (`nihongo` コマンド)

### ビルド手順

```bash
cd jp-engine_core
make          # ビルド (CMake + make)
make install  # ~/.hajimu/plugins/engine_core/ にインストール
make test     # テスト実行
make clean    # クリーン
```

### CMake 直接使用

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

## 技術仕様

- **言語**: C++20/23 (concepts, std::expected, constexpr, std::span)
- **ECS**: Archetype-based SoA (キャッシュ効率最大化)
- **並列**: Work-Stealing JobSystem (コア数 - 1 ワーカー)
- **メモリ**: Arena / Frame / Pool / Linear アロケータ
- **出力**: `engine_core.hjp` (共有ライブラリ, .hjp 拡張子)

## ファイル構成

```
jp-engine_core/
├── CMakeLists.txt
├── Makefile
├── README.md
├── hajimu.json
├── LICENSE
├── include/engine/
│   ├── engine.hpp              # アンブレラインクルード
│   ├── core/                   # 基盤 (5ファイル)
│   ├── ecs/                    # Entity Component System (7ファイル)
│   ├── input/                  # 入力 (2ファイル)
│   ├── scene/                  # シーン (3ファイル)
│   ├── resource/               # リソース管理 (3ファイル)
│   ├── render/                 # レンダリング (3ファイル)
│   ├── physics/                # 物理 (1ファイル)
│   ├── audio/                  # オーディオ (1ファイル)
│   ├── network/                # ネットワーク (1ファイル)
│   ├── script/                 # スクリプト (1ファイル)
│   └── build/                  # ビルドパイプライン (1ファイル)
├── src/
│   ├── plugin.cpp              # はじむプラグインエントリ
│   ├── core/                   # Core 実装 (4ファイル)
│   ├── ecs/                    # ECS 実装 (4ファイル)
│   ├── input/                  # Input 実装 (1ファイル)
│   ├── scene/                  # Scene 実装 (2ファイル)
│   ├── resource/               # Resource 実装 (2ファイル)
│   ├── render/                 # Render 実装 (2ファイル)
│   ├── physics/                # Physics 実装 (1ファイル)
│   ├── audio/                  # Audio 実装 (1ファイル)
│   ├── network/                # Network 実装 (1ファイル)
│   ├── script/                 # Script 実装 (1ファイル)
│   └── build/                  # Build 実装 (1ファイル)
└── tests/
    └── test_ecs.cpp            # ECS ユニットテスト
```

**ヘッダ: 28ファイル / ソース: 20ファイル / 合計: 48ファイル**

## ライセンス

MIT License — 詳細は [LICENSE](LICENSE) を参照。
