#pragma once
#include "pch.h"
#include "SLO.h"
#include "GlobalVar.h"
#include "d3dx12.h"
#include "d3dUtil.h"

namespace SLP2G
{
	struct GGeometry
	{
		using Vertex = SLO::Vertex;
		using SubmeshGeometry = SLO::SubmeshGeometry;
		using MeshGeometry = SLO::MeshGeometry;

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

		static void BuildTxtGeometry(SLO::GL* pGL, SLO::CommandObject* pCommandObject, SLO::GeometryManager* pGeometryManager, const std::string& name, const std::wstring& filename)
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

	};

}