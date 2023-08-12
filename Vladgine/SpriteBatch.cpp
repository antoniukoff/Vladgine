#include "SpriteBatch.h"
#include <algorithm>


namespace Vladgine {


	SpriteBatch::SpriteBatch() : _vbo(0), _vao(0)
	{
	}

	SpriteBatch::~SpriteBatch()
	{
	}

	void SpriteBatch::init()
	{
		createVertexArray();
	}


	// setting up our sprite batch before drawing we are coing to clear the allocated memory
	//for render batches and glyphs and are going to delete the glyphs on the heap
	void SpriteBatch::begin(GlyphSortType sortType/* = GlyphSortType::TEXTURE*/)
	{
		_sortType = sortType;
		_renderBatches.clear();
		for (int i = 0; i < _glyphs.size(); i++) {
			delete _glyphs[i];
		}
		_glyphs.clear();
	}

	void SpriteBatch::end()
	{
		sortGlyphs();
		createRenderBatches();
	}

	void SpriteBatch::draw(const glm::vec4& destRect, const glm::vec4& uvRect, GLuint texture, float depth, Color color)
	{
		// creating a sprite and setting up its attributes(vertexpos, vertexuv, vertex color, depth ,texture)
		//to the passed in parameters

		Glyph* newGlyph = new Glyph;
		newGlyph->texture = texture;
		newGlyph->depth = depth;

		newGlyph->topLeft.color = color;
		newGlyph->topLeft.setPosition(destRect.x, destRect.y + destRect.w);
		newGlyph->topLeft.setUV(uvRect.x, uvRect.y + uvRect.w);

		newGlyph->bottomLeft.color = color;
		newGlyph->bottomLeft.setPosition(destRect.x, destRect.y);
		newGlyph->bottomLeft.setUV(uvRect.x, uvRect.y);

		newGlyph->bottomRight.color = color;
		newGlyph->bottomRight.setPosition(destRect.x + destRect.z, destRect.y);
		newGlyph->bottomRight.setUV(uvRect.x + uvRect.z, uvRect.y);

		newGlyph->topRight.color = color;
		newGlyph->topRight.setPosition(destRect.x + destRect.z, destRect.y + destRect.w);
		newGlyph->topRight.setUV(uvRect.x + uvRect.z, uvRect.y + uvRect.w);

		_glyphs.push_back(newGlyph);
	}

	//renders the batches
	void SpriteBatch::renderBatch()
	{
		// bind the vao we created in the createArrayBuffer()
		glBindVertexArray(_vao);
		// loop through all the render batches
		for (int i = 0; i < _renderBatches.size(); i++) {
			//bind texture of the current array and specifies the beginning of that render batch 
			//as well as number of vertices to render until the next batch 
			glBindTexture(GL_TEXTURE_2D, _renderBatches[i].texture);
			glDrawArrays(GL_TRIANGLES, _renderBatches[i].offset, _renderBatches[i].numVertices);
		}
		glBindVertexArray(0);
		
	}

	void SpriteBatch::createRenderBatches()
	{

		// here we create batches for rendering. now it only is going to create render batches based on the texture so it is crucial to sort by texture before creating render batches to avoid multiple render batches for the same texture
		//TODO: implement depth batching. to properly organize the batches

		//allocating the total amount of memory for vertices vector to hold an appropriate number of vertices per total amount of glyphs

		std::vector<Vertex> vertices;

		vertices.resize(_glyphs.size() * 6);
	
		//return if no glyphs in the vector
		if (_glyphs.empty()) return;

		// offset used to specify the amount of verices between the render batches to minimize draw calls 
		int offset = 0;
		int cv = 0;// current vertex
		// puses back only the paramaters of the render batch constructor and sets up the first render batch
		_renderBatches.emplace_back(offset, 6, _glyphs[0]->texture);
		// upload the vertices data of the first glyph into the vertex vector
		vertices[cv++] = _glyphs[0]->topLeft;
		vertices[cv++] = _glyphs[0]->bottomLeft;
		vertices[cv++] = _glyphs[0]->bottomRight;
		vertices[cv++] = _glyphs[0]->bottomRight;
		vertices[cv++] = _glyphs[0]->topRight;
		vertices[cv++] = _glyphs[0]->topLeft;
		//increment offset by 6 vertices(offset used inly for the new render batch)
		offset += 6;
		// loop through all the glyphs
		for (int cg = 1; cg < _glyphs.size(); cg++) {
			// if the texture of the previous glyph is not the same as of the current one - create new render batch
			if (_glyphs[cg]->texture != _glyphs[cg - 1]->texture) {
				_renderBatches.emplace_back(offset, 6, _glyphs[cg]->texture);
			}
			// else increase the number of vertecies of the render batch to render
			else {
				_renderBatches.back().numVertices += 6;
			}
			//store the vertecies of subsequent glyphs in the vector
			vertices[cv++] = _glyphs[cg]->topLeft;
			vertices[cv++] = _glyphs[cg]->bottomLeft;
			vertices[cv++] = _glyphs[cg]->bottomRight;
			vertices[cv++] = _glyphs[cg]->bottomRight;
			vertices[cv++] = _glyphs[cg]->topRight;
			vertices[cv++] = _glyphs[cg]->topLeft;
			//increment offset by 6 vertices
			offset += 6;
		}
		// upload the data the data to the gpu
		glBindBuffer(GL_ARRAY_BUFFER, _vbo);
		//orphan the buffer
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex), vertices.data());

		glBindBuffer(GL_ARRAY_BUFFER, 0);


	}

	void SpriteBatch::createVertexArray()
	{
		// creates a vertex array object - storing the configuration
		//in the VAO that saves time for open gl to set up the settings 
		//about the vertex data to  render

		if (_vao == 0) {
			glGenVertexArrays(1, &_vao);
		}

		glBindVertexArray(_vao);

		if (_vbo == 0) {
			glGenBuffers(1, &_vbo);
		}
		glBindBuffer(GL_ARRAY_BUFFER, _vbo);
		
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
		//this is a color attribute pointer
		glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*)offsetof(Vertex, color));
		//uv attribute pointer
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));

		glBindVertexArray(0);


	}

	// sort teh glyphs either by depth(front to back or back to front) or by the texture
	void SpriteBatch::sortGlyphs()
	{
		switch (_sortType) {
			case GlyphSortType::BACK_TO_FRONT :
				std::stable_sort(_glyphs.begin(), _glyphs.end(), compareBackToFront);
				break;
			case GlyphSortType::FRONT_TO_BACK:
				std::stable_sort(_glyphs.begin(), _glyphs.end(), compareFrontToBack);
				break;
			case GlyphSortType::TEXTURE :
				std::stable_sort(_glyphs.begin(), _glyphs.end(), compareTexture);
				break;

		}
		
	}

	bool SpriteBatch::compareFrontToBack(Glyph* a, Glyph* b)
	{
		return (a->depth < b->depth);
	}

	bool SpriteBatch::compareBackToFront(Glyph* a, Glyph* b)
	{
		return (a->depth > b->depth);
	}

	bool SpriteBatch::compareTexture(Glyph* a, Glyph* b)
	{
		return (a->texture < b->texture);
	}

}