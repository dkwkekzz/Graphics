#pragma once
#include "pch.h"
#include "SLO.h"
#include "GlobalVar.h"
#include "Pures.h"
#include "d3dx12.h"
#include "d3dUtil.h"
#include "GeometryGenerator.h"


namespace SLP2G
{
	struct GDelayCall
	{
		FASTCALL CreateTexture(SLO::TextureManager* pTextureManager, LPCSTR name, LPCWSTR filename)
		{
			SLO::TextureCreateDesc desc;
			desc.name = name;
			desc.filename = filename;
			desc.srvDesc = {};
			desc.srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			//desc.srvDesc.Format = res->GetDesc().Format;
			desc.srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			desc.srvDesc.Texture2D.MostDetailedMip = 0;
			desc.srvDesc.Texture2D.MipLevels = -1;

			pTextureManager->waitqueue.emplace(desc);
		}

		FASTCALL CreateTextureArray(SLO::TextureManager* pTextureManager, LPCSTR name, LPCWSTR filename)
		{
			SLO::TextureCreateDesc desc;
			desc.name = name;
			desc.filename = filename;
			desc.srvDesc = {};
			desc.srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			//desc.srvDesc.Format = res->GetDesc().Format;
			desc.srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			desc.srvDesc.Texture2D.MostDetailedMip = 0;
			desc.srvDesc.Texture2D.MipLevels = -1;
			desc.srvDesc.Texture2DArray.MostDetailedMip = 0;
			desc.srvDesc.Texture2DArray.MipLevels = -1;
			desc.srvDesc.Texture2DArray.FirstArraySlice = 0;

			pTextureManager->waitqueue.emplace(desc);
		}

		FASTCALL CreateMesh(SLO::GeometryManager* pGeometryManager, LPCSTR name, LPCWSTR filename)
		{
			pGeometryManager->waitqueue.push(std::make_pair(name, filename));
		}
	};

	struct GRenderer
	{
		static void CreateRenderItem(SLO::Actor* pActor, SLO::Material* pMaterial, SLO::RenderItemManager* pRenderItemManager, 
			SLO::RenderLayer layer, 
			int submesh = 0, 
			D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
		{
			using RenderItem = SLO::RenderItem;

			auto ritem = std::make_unique<RenderItem>();
			//ritem->World = MathHelper::Identity4x4();
			//ritem->TexTransform = MathHelper::Identity4x4();
			//ritem->ObjCBIndex = pRenderItemManager->itemCount++;
			ritem->Act = pActor;
			ritem->Mat = pMaterial;
			//ritem->Geo = pMeshGeometry;
			ritem->PrimitiveType = primitiveType;
			ritem->IndexCount = pActor->Geo->Submeshes[submesh].IndexCount;
			ritem->StartIndexLocation = pActor->Geo->Submeshes[submesh].StartIndexLocation;
			ritem->BaseVertexLocation = pActor->Geo->Submeshes[submesh].BaseVertexLocation;
			pRenderItemManager->ritemLayer[(int)layer].items.push_back(ritem.get());
			pRenderItemManager->allritems.emplace_back(std::move(ritem));
		}
	};

	struct GActor
	{
		static void CreateActor(SLO::ActorCollection* pActorCollection, SLO::MeshGeometry* pMeshGeometry, int& generatedId)
		{
			generatedId = pActorCollection->actors.size();

			auto Actor = std::make_unique<SLO::Actor>();
			Actor->ObjCBIndex = generatedId;
			Actor->World = MathHelper::Identity4x4();
			Actor->TexTransform = MathHelper::Identity4x4();
			Actor->NumFramesDirty = Global::FRAME_RESOURCE_COUNT;
			Actor->Geo = pMeshGeometry;
			pActorCollection->actors.emplace_back(std::move(Actor));
		}

		static void TransformActor(SLO::ActorCollection* pActorCollection, int id, const DirectX::XMMATRIX& matrix)
		{
			auto* item = pActorCollection->actors[id].get();
			XMMATRIX world = XMLoadFloat4x4(&item->World);
			XMStoreFloat4x4(&(item->World), DirectX::XMMatrixMultiply(world, matrix));
			item->NumFramesDirty = Global::FRAME_RESOURCE_COUNT;
		}
	};

	struct GMesh
	{
		using Vertex = SLO::Vertex;
		using SubmeshGeometry = SLO::SubmeshGeometry;
		using MeshGeometry = SLO::MeshGeometry;

		static void BuildTxtGeometry(SLO::GL* pGL, SLO::CommandObject* pCommandObject, SLO::GeometryManager* pGeometryManager, 
			const std::string& name, const std::wstring& filename)
		{
			std::ifstream fin(filename);

			if (!fin)
			{
				wchar_t message[512];
				swprintf_s(message, L"%s not found.", filename.c_str());
				MessageBox(0, message, 0, 0);
				return;
			}

			UINT vcount = 0;
			UINT tcount = 0;
			std::string ignore;

			fin >> ignore >> vcount;
			fin >> ignore >> tcount;
			fin >> ignore >> ignore >> ignore >> ignore;

			std::vector<Vertex> vertices(vcount);
			for (UINT i = 0; i < vcount; ++i)
			{
				fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
				fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;

				// Model does not have texture coordinates, so just zero them out.
				vertices[i].TexC = { 0.0f, 0.0f };
			}

			fin >> ignore;
			fin >> ignore;
			fin >> ignore;

			std::vector<std::int32_t> indices(3 * tcount);
			for (UINT i = 0; i < tcount; ++i)
			{
				fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
			}

			fin.close();

			//
			// Pack the indices of all the meshes into one index buffer.
			//

			const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

			const UINT ibByteSize = (UINT)indices.size() * sizeof(std::int32_t);

			auto geo = std::make_unique<MeshGeometry>();
			geo->Name = name;

			ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
			CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

			ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
			CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

			geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(pGL->d3dDevice.Get(),
				pCommandObject->commandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

			geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(pGL->d3dDevice.Get(),
				pCommandObject->commandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

			geo->VertexByteStride = sizeof(Vertex);
			geo->VertexBufferByteSize = vbByteSize;
			geo->IndexFormat = DXGI_FORMAT_R32_UINT;
			geo->IndexBufferByteSize = ibByteSize;

			SubmeshGeometry submesh;
			submesh.IndexCount = (UINT)indices.size();
			submesh.StartIndexLocation = 0;
			submesh.BaseVertexLocation = 0;

			geo->Submeshes.emplace_back(submesh);

			pGeometryManager->geometries[geo->Name] = std::move(geo);
		}

		static void BuildRoomGeometry(SLO::GL* pGL, SLO::CommandObject* pCommandObject, SLO::GeometryManager* pGeometryManager)
		{
			// Create and specify geometry.  For this sample we draw a floor
			// and a wall with a mirror on it.  We put the floor, wall, and
			// mirror geometry in one vertex buffer.
			//
			//   |--------------|
			//   |              |
			//   |----|----|----|
			//   |Wall|Mirr|Wall|
			//   |    | or |    |
			//   /--------------/
			//  /   Floor      /
			// /--------------/

			std::array<Vertex, 20> vertices =
			{
				// Floor: Observe we tile texture coordinates.
				Vertex(-3.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 0.0f, 4.0f), // 0 
				Vertex(-3.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f),
				Vertex(7.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 4.0f, 0.0f),
				Vertex(7.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 4.0f, 4.0f),

				// Wall: Observe we tile texture coordinates, and that we
				// leave a gap in the middle for the mirror.
				Vertex(-3.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f), // 4
				Vertex(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
				Vertex(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 0.0f),
				Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 2.0f),

				Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f), // 8 
				Vertex(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
				Vertex(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 0.0f),
				Vertex(7.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 2.0f),

				Vertex(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f), // 12
				Vertex(-3.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
				Vertex(7.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 0.0f),
				Vertex(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 1.0f),

				// Mirror
				Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f), // 16
				Vertex(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
				Vertex(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f),
				Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f)
			};

			std::array<std::int16_t, 30> indices =
			{
				// Floor
				0, 1, 2,
				0, 2, 3,

				// Walls
				4, 5, 6,
				4, 6, 7,

				8, 9, 10,
				8, 10, 11,

				12, 13, 14,
				12, 14, 15,

				// Mirror
				16, 17, 18,
				16, 18, 19
			};

			SubmeshGeometry floorSubmesh;
			floorSubmesh.IndexCount = 6;
			floorSubmesh.StartIndexLocation = 0;
			floorSubmesh.BaseVertexLocation = 0;

			SubmeshGeometry wallSubmesh;
			wallSubmesh.IndexCount = 18;
			wallSubmesh.StartIndexLocation = 6;
			wallSubmesh.BaseVertexLocation = 0;

			SubmeshGeometry mirrorSubmesh;
			mirrorSubmesh.IndexCount = 6;
			mirrorSubmesh.StartIndexLocation = 24;
			mirrorSubmesh.BaseVertexLocation = 0;

			const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
			const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

			auto geo = std::make_unique<MeshGeometry>();
			geo->Name = "roomGeo";

			ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
			CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

			ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
			CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

			geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(pGL->d3dDevice.Get(),
				pCommandObject->commandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

			geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(pGL->d3dDevice.Get(),
				pCommandObject->commandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

			geo->VertexByteStride = sizeof(Vertex);
			geo->VertexBufferByteSize = vbByteSize;
			geo->IndexFormat = DXGI_FORMAT_R16_UINT;
			geo->IndexBufferByteSize = ibByteSize;

			geo->Submeshes.emplace_back(floorSubmesh);
			geo->Submeshes.emplace_back(wallSubmesh);
			geo->Submeshes.emplace_back(mirrorSubmesh);

			pGeometryManager->geometries[geo->Name] = std::move(geo);
		}

		static void BuildLandGeometry(SLO::GL* pGL, SLO::CommandObject* pCommandObject, SLO::GeometryManager* pGeometryManager)
		{
			GeometryGenerator geoGen;
			GeometryGenerator::MeshData grid = geoGen.CreateGrid(160.0f, 160.0f, 50, 50);

			//
			// Extract the vertex elements we are interested and apply the height function to
			// each vertex.  In addition, color the vertices based on their height so we have
			// sandy looking beaches, grassy low hills, and snow mountain peaks.
			//

			std::vector<Vertex> vertices(grid.Vertices.size());
			for (size_t i = 0; i < grid.Vertices.size(); ++i)
			{
				auto& p = grid.Vertices[i].Position;
				vertices[i].Pos = p;
				vertices[i].Pos.y = GetHillsHeight(p.x, p.z);
				vertices[i].Normal = GetHillsNormal(p.x, p.z);
				vertices[i].TexC = grid.Vertices[i].TexC;
			}

			const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

			std::vector<std::uint16_t> indices = grid.GetIndices16();
			const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

			auto geo = std::make_unique<MeshGeometry>();
			geo->Name = "landGeo";

			ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
			CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

			ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
			CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

			geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(pGL->d3dDevice.Get(),
				pCommandObject->commandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

			geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(pGL->d3dDevice.Get(),
				pCommandObject->commandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

			geo->VertexByteStride = sizeof(Vertex);
			geo->VertexBufferByteSize = vbByteSize;
			geo->IndexFormat = DXGI_FORMAT_R16_UINT;
			geo->IndexBufferByteSize = ibByteSize;

			SubmeshGeometry submesh;
			submesh.IndexCount = (UINT)indices.size();
			submesh.StartIndexLocation = 0;
			submesh.BaseVertexLocation = 0;

			geo->Submeshes.emplace_back(submesh);

			pGeometryManager->geometries["landGeo"] = std::move(geo);
		}

		static void BuildWavesGeometry(SLO::GL* pGL, SLO::CommandObject* pCommandObject, SLO::GeometryManager* pGeometryManager, Waves* pWaves)
		{
			std::vector<std::uint16_t> indices(3 * pWaves->TriangleCount()); // 3 indices per face
			assert(pWaves->VertexCount() < 0x0000ffff);

			// Iterate over each quad.
			int m = pWaves->RowCount();
			int n = pWaves->ColumnCount();
			int k = 0;
			for (int i = 0; i < m - 1; ++i)
			{
				for (int j = 0; j < n - 1; ++j)
				{
					indices[k] = i * n + j;
					indices[k + 1] = i * n + j + 1;
					indices[k + 2] = (i + 1) * n + j;

					indices[k + 3] = (i + 1) * n + j;
					indices[k + 4] = i * n + j + 1;
					indices[k + 5] = (i + 1) * n + j + 1;

					k += 6; // next quad
				}
			}

			UINT vbByteSize = pWaves->VertexCount() * sizeof(Vertex);
			UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

			auto geo = std::make_unique<MeshGeometry>();
			geo->Name = "waterGeo";

			// Set dynamically.
			geo->VertexBufferCPU = nullptr;
			geo->VertexBufferGPU = nullptr;

			ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
			CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

			geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(pGL->d3dDevice.Get(),
				pCommandObject->commandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

			geo->VertexByteStride = sizeof(Vertex);
			geo->VertexBufferByteSize = vbByteSize;
			geo->IndexFormat = DXGI_FORMAT_R16_UINT;
			geo->IndexBufferByteSize = ibByteSize;

			SubmeshGeometry submesh;
			submesh.IndexCount = (UINT)indices.size();
			submesh.StartIndexLocation = 0;
			submesh.BaseVertexLocation = 0;

			geo->Submeshes.emplace_back(submesh);

			pGeometryManager->geometries["waterGeo"] = std::move(geo);
		}

		static void BuildBoxGeometry(SLO::GL* pGL, SLO::CommandObject* pCommandObject, SLO::GeometryManager* pGeometryManager)
		{
			GeometryGenerator geoGen;
			GeometryGenerator::MeshData box = geoGen.CreateBox(8.0f, 8.0f, 8.0f, 3);

			std::vector<Vertex> vertices(box.Vertices.size());
			for (size_t i = 0; i < box.Vertices.size(); ++i)
			{
				auto& p = box.Vertices[i].Position;
				vertices[i].Pos = p;
				vertices[i].Normal = box.Vertices[i].Normal;
				vertices[i].TexC = box.Vertices[i].TexC;
			}

			const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

			std::vector<std::uint16_t> indices = box.GetIndices16();
			const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

			auto geo = std::make_unique<MeshGeometry>();
			geo->Name = "boxGeo";

			ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
			CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

			ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
			CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

			geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(pGL->d3dDevice.Get(),
				pCommandObject->commandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

			geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(pGL->d3dDevice.Get(),
				pCommandObject->commandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

			geo->VertexByteStride = sizeof(Vertex);
			geo->VertexBufferByteSize = vbByteSize;
			geo->IndexFormat = DXGI_FORMAT_R16_UINT;
			geo->IndexBufferByteSize = ibByteSize;

			SubmeshGeometry submesh;
			submesh.IndexCount = (UINT)indices.size();
			submesh.StartIndexLocation = 0;
			submesh.BaseVertexLocation = 0;

			geo->Submeshes.emplace_back(submesh);

			pGeometryManager->geometries["boxGeo"] = std::move(geo);
		}

		static void BuildTreeSpritesGeometry(SLO::GL* pGL, SLO::CommandObject* pCommandObject, SLO::GeometryManager* pGeometryManager)
		{
			struct TreeSpriteVertex
			{
				XMFLOAT3 Pos;
				XMFLOAT2 Size;
			};

			static const int treeCount = 16;
			std::array<TreeSpriteVertex, 16> vertices;
			for (UINT i = 0; i < treeCount; ++i)
			{
				float x = MathHelper::RandF(-45.0f, 45.0f);
				float z = MathHelper::RandF(-45.0f, 45.0f);
				float y = GetHillsHeight(x, z);

				// Move tree slightly above land height.
				y += 8.0f;

				vertices[i].Pos = XMFLOAT3(x, y, z);
				vertices[i].Size = XMFLOAT2(20.0f, 20.0f);
			}

			std::array<std::uint16_t, 16> indices =
			{
				0, 1, 2, 3, 4, 5, 6, 7,
				8, 9, 10, 11, 12, 13, 14, 15
			};

			const UINT vbByteSize = (UINT)vertices.size() * sizeof(TreeSpriteVertex);
			const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

			auto geo = std::make_unique<MeshGeometry>();
			geo->Name = "treeSpritesGeo";

			ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
			CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

			ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
			CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

			geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(pGL->d3dDevice.Get(),
				pCommandObject->commandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

			geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(pGL->d3dDevice.Get(),
				pCommandObject->commandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

			geo->VertexByteStride = sizeof(TreeSpriteVertex);
			geo->VertexBufferByteSize = vbByteSize;
			geo->IndexFormat = DXGI_FORMAT_R16_UINT;
			geo->IndexBufferByteSize = ibByteSize;

			SubmeshGeometry submesh;
			submesh.IndexCount = (UINT)indices.size();
			submesh.StartIndexLocation = 0;
			submesh.BaseVertexLocation = 0;

			geo->Submeshes.emplace_back(submesh);

			pGeometryManager->geometries["treeSpritesGeo"] = std::move(geo);
		}

	};

}